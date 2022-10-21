#include "mio.hpp"
#include "robin_hood.h"
#include "veque.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <omp.h>
#include <sstream>
#include <string>
#include <vector>

//#define DEBUG

using std::string;
using std::vector;

template <typename E> constexpr auto to_underlying(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

enum class Color : int8_t { green, yellow, blue, invalid };

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
  bool operator==(const Troon &other) const { return id == other.id; }
  bool operator!=(const Troon &other) const { return !(*this == other); }

  string to_string(const string &station_name) const {
    return (line == Color::green    ? "g"
            : line == Color::yellow ? "y"
                                    : "b") +
           std::to_string(id) + "-" + station_name;
  }
};

const Troon INVALID_TROON{-1, -1, Color::invalid, false};
std::array<vector<string>, 3> troon_strs;
std::array<int, 3> troon_strs_idx;

struct WaitingTroon {
  int time;
  Troon troon;
  WaitingTroon(int _time, Troon _troon) : time{_time}, troon{_troon} {}
  bool operator<(const WaitingTroon &other) const {
    if (time != other.time)
      return time < other.time;
    return troon < other.troon;
  }
};

struct Link {
  int16_t from;
  int16_t to;
  int len;
  int popularity;
  int prev_sz = 0;
  Troon transiting{INVALID_TROON};
  Troon loading{INVALID_TROON};
  veque::veque<WaitingTroon> holding_area;
  std::unique_ptr<std::mutex> buf_lock;

  Link(int16_t _from, int16_t _to, int _len, int _popularity)
      : from{_from}, to{_to}, len{_len}, popularity{_popularity} {
    buf_lock = std::make_unique<std::mutex>();
  }

  void run(int tick,
           const vector<robin_hood::unordered_map<int, Link *>> &links,
           const std::array<std::array<vector<int>, 3>, 2> &station_lists) {
    // Process transiting troon
    if (transiting.timer == tick) {
      int next_to =
          station_lists[transiting.forward][to_underlying(transiting.line)][to];
      if (next_to == -1) {
        transiting.forward = !transiting.forward;
        next_to = from;
      }
      links[to].find(next_to)->second->enqueue_troon(tick, transiting);
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
    holding_area.emplace_back(tick, troon);
  }

  void flush_buffers(int tick) {
    if (!holding_area.empty() && loading == INVALID_TROON) {
      std::sort(holding_area.begin() + prev_sz, holding_area.end());
      loading = holding_area.front().troon;
      loading.timer = std::max(1 + popularity + tick, transiting.timer) + 1;
      holding_area.pop_front();
      prev_sz = holding_area.size();
    }
  }

  void current_troons(int idx, const string &station_name,
                      const string &to_station_name) {
    for (auto cur : holding_area) {
      const int line_idx = to_underlying(cur.troon.line);
      if (line_idx == idx) {
        troon_strs[idx][troon_strs_idx[idx]++] =
            cur.troon.to_string(station_name) + "#";
      }
    }
    if (loading != INVALID_TROON) {
      const int line_idx = to_underlying(loading.line);
      if (line_idx == idx) {
        troon_strs[idx][troon_strs_idx[idx]++] =
            loading.to_string(station_name) + "%";
      }
    }
    if (transiting != INVALID_TROON) {
      const int line_idx = to_underlying(transiting.line);
      if (line_idx == idx) {
        troon_strs[idx][troon_strs_idx[idx]++] =
            transiting.to_string(station_name) + "->" + to_station_name;
      }
    }
  }

  void spawn_troon(int tick, Troon &&troon) {
    holding_area.emplace_back(tick, troon);
  }
};

int next_troon_id = 0;

/**
 * Note: for all_station_names and `num_troons`, the top-level indices follow
 * the order: green, yellow, blue. This holds true for basically all arrays that
 * contain information about all 3 lines.
 */
void simulate(const vector<string> &station_names,
              const vector<robin_hood::unordered_map<int, Link *>> &links,
              vector<Link> &flattened_links,
              const std::array<Link *, 6> &spawn_locations,
              const std::array<std::array<vector<int>, 3>, 2> &station_lists,
              int ticks, std::array<int, 3> &num_troons, int num_lines) {

  for (int tick{}; tick < ticks; ++tick) {
    for (int i = 0; i < 3 && num_troons[i]; ++i) {
      for (int j = 0; num_troons[i] && j < 2; ++j) {
        spawn_locations[i * 2 + j]->spawn_troon(
            tick, Troon{next_troon_id++, static_cast<Color>(i), !j});
        --num_troons[i];
      }
    }

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < flattened_links.size(); ++i) {
      flattened_links[i].run(tick, links, station_lists);
    }

    // Final flush of buffers
#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < flattened_links.size(); ++i) {
      flattened_links[i].flush_buffers(tick);
    }

    if (tick + num_lines >= ticks) {
      std::memset(troon_strs_idx.begin(), 0, sizeof troon_strs_idx);

#pragma omp parallel for num_threads(3) schedule(static)
      for (int line_idx = 0; line_idx < 3; ++line_idx) {
        for (auto &link : flattened_links) {
          link.current_troons(line_idx, station_names[link.from],
                              station_names[link.to]);
        }
      }

#pragma omp parallel for num_threads(3) schedule(static)
      for (int line_idx = 0; line_idx < 3; ++line_idx) {
        std::sort(troon_strs[line_idx].begin(),
                  troon_strs[line_idx].begin() + troon_strs_idx[line_idx]);
      }

      std::cout << tick << ": ";
      for (int line_idx : {2, 0, 1}) {
        for (int i = 0; i < troon_strs_idx[line_idx]; ++i) {
          std::cout << troon_strs[line_idx][i] << " ";
        }
      }
      std::cout << '\n';
    }
  }
}

int read_int(const char *&it) {
  // note: we don't care about negatives
  int res = 0;
  for (int c = *it++; std::isdigit(c); c = *it++) {
    res = res * 10 + c - '0';
  }
  return res;
}

string read_string(const char *&it) {
  const auto start_it = it;
  for (char c = *it++; !std::isspace(c); c = *it++)
    ;
  return string(start_it, it - start_it - 1);
}

string read_line(const char *&it) {
  const auto start_it = it;
  for (char c = *it++; c != '\n'; c = *it++)
    ;
  return string(start_it, it - start_it - 1);
}

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    std::cerr << argv[0] << " <input_file>\n";
    std::exit(1);
  }

  omp_set_num_threads(std::max(8, omp_get_num_procs() / 2));

  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  mio::mmap_source mmap(argv[1]);
  auto mmap_it = mmap.begin();

  // Read S
  int S = read_int(mmap_it);

  // Read station names.
  string station;
  std::vector<string> station_names{};
  robin_hood::unordered_flat_map<std::string_view, int> station_ids;
  station_names.reserve(S);
  station_ids.reserve(S);
  for (int i = 0; i < S; ++i) {
    station = read_string(mmap_it);
    station_names.push_back(std::move(station));
    station_ids[station_names.back()] = i;
  }

  // Read P popularity
  int p;
  vector<int> popularities{};
  popularities.reserve(S);
  for (int i = 0; i < S; ++i) {
    p = read_int(mmap_it);
    popularities.emplace_back(p);
  }

  vector<robin_hood::unordered_flat_map<int, Link *>> links(S);
  vector<Link> flattened_links;
  flattened_links.reserve(S * S);
  for (int src{}; src < S; ++src) {
    for (int dst{}; dst < S; ++dst) {
      int len = read_int(mmap_it);
      if (len) {
        flattened_links.emplace_back(src, dst, len, popularities[src]);
        links[src][dst] = &flattened_links.back();
      }
    }
  }

  string stations_buf;
  // [forward = 1/backward = 0][green = 0, yellow = 1, blue = 2]
  std::array<std::array<vector<int>, 3>, 2> station_lists;
  vector<int> all_station_ids(S, -1);
  std::array<Link *, 6> spawn_locations;
  for (int i = 0; i < 3; ++i) {
    stations_buf = read_line(mmap_it);
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
  int N = read_int(mmap_it);

  // g,y,b number of trains per line
  std::array<int, 3> num_troons;
  for (int i{}; i < 3; ++i) {
    num_troons[i] = read_int(mmap_it);
    troon_strs[i].resize(num_troons[i]);
  }

  int num_lines = read_int(mmap_it);

  simulate(station_names, links, flattened_links, spawn_locations,
           station_lists, N, num_troons, num_lines);

  return 0;
}
