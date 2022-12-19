all:
	gcc -o fntf_xmpl_cp fntf_xmpl_cp.cpp -lstdc++ -lsqlite3
	gcc -o encrypt encrypt.c