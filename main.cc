#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

//#define DEBUG

using std::string;
using std::vector;

template <typename E> constexpr auto to_underlying(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

enum class Color { green, yellow, blue, invalid };

struct Troon {
  int id;
  int timer;
  Color line;
  bool forward;

  Troon(int _id, int _timer, Color _line, bool _forward)
      : id{_id}, timer{_timer}, line{_line}, forward{_forward} {}
  Troon(int _id, Color _line, bool _forward)
      : id{_id}, line{_line}, forward{_forward} {}

  bool operator<(const Troon &other) const { return id < other.id; }
  bool operator>(const Troon &other) const { return id > other.id; }
  bool operator==(const Troon &other) const { return id == other.id; }
  bool operator!=(const Troon &other) const { return !(*this == other); }

  string to_string(const string &station_name) const {
    return (line == Color::green    ? "g"
            : line == Color::yellow ? "y"
                                    : "b") +
           std::to_string(id) + "-" + station_name;
  }
};

Troon INVALID_TROON{-1, std::numeric_limits<int>::max(), Color::invalid, false};
vector<string> troon_strings;
int troon_strings_idx = 0;

struct WaitingTroon {
  int time;
  Troon troon;
  WaitingTroon(int _time, Troon _troon) : time{_time}, troon{_troon} {}
  bool operator>(const WaitingTroon &other) const {
    if (time != other.time)
      return time > other.time;
    return troon > other.troon;
  }
};

struct Link {
  int from;
  int to;
  int len;
  int popularity;
  Troon transiting{INVALID_TROON};
  Troon loading{INVALID_TROON};
  std::priority_queue<WaitingTroon, vector<WaitingTroon>,
                      std::greater<WaitingTroon>>
      holding_area;
  std::unique_ptr<std::mutex> buf_lock;

  Link(int _from, int _to, int _len, int _popularity)
      : from{_from}, to{_to}, len{_len}, popularity{_popularity} {
    buf_lock = std::make_unique<std::mutex>();
  }

  void run(int tick, vector<vector<Link *>> &links,
           std::array<std::array<vector<int>, 3>, 2> &station_lists) {
    // Process transiting troon
    if (transiting.timer == tick) {
      int next_to =
          station_lists[transiting.forward][to_underlying(transiting.line)][to];
      if (next_to == -1) {
        transiting.forward = !transiting.forward;
        next_to = from;
      }
      links[to][next_to]->enqueue_troon(tick, transiting);
      transiting = INVALID_TROON;
    }

    // Note: when a tick ends, either pre_transit == nullptr
    // or loading == nullptr.
    if (loading.timer == tick) {
      transiting = loading;
      transiting.timer = tick + len;
      loading = INVALID_TROON;
    }
  }

  void enqueue_troon(int tick, Troon troon) {
    const std::lock_guard<std::mutex> lock{*buf_lock};
    holding_area.emplace(tick, troon);
  }

  void flush_buffers(int tick) {
    if (!holding_area.empty() && loading == INVALID_TROON) {
      loading = holding_area.top().troon;
      loading.timer =
          std::max(1 + popularity + tick,
                   transiting.timer == std::numeric_limits<int>::max()
                       ? 0
                       : transiting.timer) +
          1;
      holding_area.pop();
    }
  }

  void current_troons(const string &station_name,
                      const string &to_station_name) {
    vector<WaitingTroon> holding_area_buf;
    while (!holding_area.empty()) {
      auto &cur = holding_area.top();
      troon_strings[troon_strings_idx++] =
          cur.troon.to_string(station_name) + "#";
      holding_area_buf.push_back(cur);
      holding_area.pop();
    }
    for (auto &x : holding_area_buf) {
      holding_area.push(x);
    }
    if (loading != INVALID_TROON) {
      troon_strings[troon_strings_idx++] =
          loading.to_string(station_name) + "%";
    }
    if (transiting != INVALID_TROON) {
      troon_strings[troon_strings_idx++] =
          transiting.to_string(station_name) + "->" + to_station_name;
    }
  }

  void spawn_troon(int tick, Troon &&troon) {
    holding_area.emplace(tick, troon);
  }
};

int next_troon_id = 0;

/**
 * Note: for all_station_names and `num_troons`, the top-level indices follow
 * the order: green, yellow, blue. This holds true for basically all arrays that
 * contain information about all 3 lines.
 */
void simulate(const vector<string> &station_names,
              vector<vector<Link *>> &links, vector<Link> &flattened_links,
              vector<Link *> &spawn_locations,
              std::array<std::array<vector<int>, 3>, 2> &station_lists,
              int ticks, std::array<int, 3> &num_troons, int num_lines) {

  for (int tick{}; tick < ticks; ++tick) {
    for (int i = 0; i < 3 && num_troons[i]; ++i) {
      for (int j = 0; num_troons[i] && j < 2; ++j) {
        spawn_locations[i * 2 + j]->spawn_troon(
            tick, Troon{next_troon_id++, static_cast<Color>(i), !j});
        --num_troons[i];
      }
    }

#pragma omp parallel for num_threads(8)
    for (size_t i = 0; i < flattened_links.size(); ++i) {
      flattened_links[i].run(tick, links, station_lists);
    }

    // Final flush of buffers
#pragma omp parallel for num_threads(8)
    for (size_t i = 0; i < flattened_links.size(); ++i) {
      flattened_links[i].flush_buffers(tick);
    }

    if (tick + num_lines >= ticks) {
      troon_strings_idx = 0;
      for (auto &link : flattened_links) {
        link.current_troons(station_names[link.from], station_names[link.to]);
      }
      std::sort(troon_strings.begin(),
                troon_strings.begin() + troon_strings_idx);

      std::cout << tick << ": ";
      for (int i = 0; i < troon_strings_idx; ++i) {
        std::cout << troon_strings[i] << " ";
      }
      std::cout << '\n';
    }
  }
}

int main(int argc, char const *argv[]) {
  using std::cout;

  if (argc < 2) {
    std::cerr << argv[0] << " <input_file>\n";
    std::exit(1);
  }

  std::ifstream ifs(argv[1], std::ios_base::in);
  if (!ifs.is_open()) {
    std::cerr << "Failed to open " << argv[1] << '\n';
    std::exit(2);
  }

  // Read S
  int S;
  ifs >> S;

  // Read station names.
  string station;
  std::vector<string> station_names{};
  std::unordered_map<std::string_view, int> station_ids;
  station_names.reserve(S);
  station_ids.reserve(S);
  for (int i = 0; i < S; ++i) {
    ifs >> station;
    station_names.push_back(std::move(station));
    station_ids[station_names.back()] = i;
  }

  // Read P popularity
  int p;
  vector<int> popularities{};
  popularities.reserve(S);
  for (int i = 0; i < S; ++i) {
    ifs >> p;
    popularities.emplace_back(p);
  }

  vector<vector<Link *>> links(S, vector<Link *>(S, nullptr));
  vector<Link> flattened_links;
  flattened_links.reserve(S * S);
  for (int src{}; src < S; ++src) {
    for (int dst{}; dst < S; ++dst) {
      int len;
      ifs >> len;
      if (len) {
        flattened_links.emplace_back(src, dst, len, popularities[src]);
        links[src][dst] = &flattened_links.back();
      }
    }
  }

  ifs.ignore();

  string stations_buf;
  // [forward = 1/backward = 0][green = 0, yellow = 1, blue = 2]
  std::array<std::array<vector<int>, 3>, 2> station_lists;
  vector<int> all_station_ids(S, -1);
  vector<Link *> spawn_locations(6);
  for (int i = 0; i < 3; ++i) {
    std::getline(ifs, stations_buf);
    std::stringstream ss{stations_buf};
    int sz = 0;
    while (ss >> stations_buf) {
      all_station_ids[sz] = station_ids[stations_buf];
      ++sz;
    }

    spawn_locations[i * 2] = links[all_station_ids[0]][all_station_ids[1]];
    spawn_locations[i * 2 + 1] =
        links[all_station_ids[sz - 1]][all_station_ids[sz - 2]];

    auto &_next_station = station_lists[1][i];
    auto &_prev_station = station_lists[0][i];
    _next_station.assign(S, -1);
    _prev_station.assign(S, -1);
    for (int j = 0; j + 1 < sz; ++j) {
      int prev_idx = all_station_ids[j];
      int next_idx = all_station_ids[j + 1];
      _next_station[prev_idx] = next_idx;
      _prev_station[next_idx] = prev_idx;
    }
  }

  // N time ticks
  int N;
  ifs >> N;

  // g,y,b number of trains per line
  std::array<int, 3> num_troons;
  for (int i{}; i < 3; ++i) {
    ifs >> num_troons[i];
  }

  troon_strings.resize(num_troons[0] + num_troons[1] + num_troons[2]);

  int num_lines;
  ifs >> num_lines;

  simulate(station_names, links, flattened_links, spawn_locations,
           station_lists, N, num_troons, num_lines);

  return 0;
}
