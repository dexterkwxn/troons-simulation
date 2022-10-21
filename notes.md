# Troons (CS3210 Assignment 1)

## Notes
- 1 platform and 1 holding area per outgoing link
- If outgoing link is empty, wait 1 tick before leaving
- Holding area queue is sorted by ID
- 1 tick for opening doors
- num_popularity ticks for loading passengers and closing doors
- Upon arrival at terminal station, goes to the link in the opposite direction 

## Notes
- 1 outer for loop for ticks
- 1 inner for loop for each station
- 1 final inner for loop to move all the holding area buffers to the station's actual holding area
- 1 counter for train travelling on link, 1 for platform and waiting for free link, 1 for platform and opening+closing doors

## Benchmark
17k stations
200k trains per line
100k ticks
5 output lines
