set terminal qt size 1000,700 enhanced font 'Verdana,12'
set title 'Random Read Performance'
set xlabel 'Iteration'
set ylabel 'Time (µs)'
set grid
set key outside right top
plot 'temp_graph_0.dat' title '8MB Read Latency' with lines
pause mouse close
