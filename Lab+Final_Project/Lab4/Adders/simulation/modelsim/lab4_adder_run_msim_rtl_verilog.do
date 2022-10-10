transcript on
if {[file exists rtl_work]} {
	vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work

vlog -sv -work work +incdir+D:/ECE385/Lab4/Adders {D:/ECE385/Lab4/Adders/HexDriver.sv}
vlog -sv -work work +incdir+D:/ECE385/Lab4/Adders {D:/ECE385/Lab4/Adders/carry_select_adder.sv}
vlog -sv -work work +incdir+D:/ECE385/Lab4/Adders {D:/ECE385/Lab4/Adders/lab4_adders_toplevel.sv}

vlog -sv -work work +incdir+D:/ECE385/Lab4/Adders {D:/ECE385/Lab4/Adders/testbench.sv}

vsim -t 1ps -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cycloneive_ver -L rtl_work -L work -voptargs="+acc"  testbench

add wave *
view structure
view signals
run 2000 ns
