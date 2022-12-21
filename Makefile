all:
	gcc -o fntf_xmpl_cp fntf_xmpl_cp.cpp sqlite3.c -lpthread -ldl -lm -lstdc++
	gcc -o encrypt encrypt.c