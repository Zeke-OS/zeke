# Gnuplot script file for plotting data in file plot.txt"
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
unset title
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set xlabel "links"
set ylabel "ms"
set terminal postscript eps
set output 'dh_link.eps'
plot "dh_link.txt" using 1:2 title "dh_link exec time" with linespoints
