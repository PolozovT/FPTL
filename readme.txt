������ ������� ��� MSVS:

1) ��������� � ������ boost (http://www.boost.org/users/download/)
2) � ���������� ������� ������� ���������� ���� �� boost (������->��������->�������� VC++)
3) ������� jemalloc (Thirdparty)
	�) ���������� cygwin (https://cygwin.com/install.html) � ������������: 
		- autoconf
		- autogen
		- grep
		- gawk
		- sed
		- bison
		- flex
	b) �������� ���� �� cygwin\bin � ���������� �����
	�) ������� ������� ������������ (command promt vs), ������� � ���������� jemalloc 
	d) ��������� sh -c "CC=cl ./autogen.sh"
	e) ������� ������ msvc\jemalloc_vc2015.sln
	f) ��������� dll � ����� � ��� ���������������� ����� � Thirdparty\Lib
4) ��������� ������ fptl
