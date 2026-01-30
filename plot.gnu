# plot.gnu — bar chart of register values
set title "RISC-V Register Values (after program)"
set xlabel "Register"
set ylabel "Value"
set grid y
set grid x
set style data histograms
set style fill solid 1.0 border -1
set boxwidth 0.8
set xtics rotate by 0

# If values can be negative, remove the next line
set yrange [0:*]

# Using: column 2 as value, column 1 as label
plot "reg_values.dat" using 2:xtic(1) title "x0..x7"
