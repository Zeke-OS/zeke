# Gnuplot script file for plotting data in file plot.txt"
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
unset title
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set xlabel "links"
set ylabel "us"
set terminal postscript eps
set output 'dh_lookup.eps'
plot "dh_lookup.txt" using 1:2 title "lookup time" with linespoints, '' using 1:3 title "% found" axes x1y2 w lines
