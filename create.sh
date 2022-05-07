CURDIR=`pwd`
DIR=`pwd`


find -L $DIR/compat $DIR/lib $DIR/maint $DIR/tests $DIR/win32 -name "*.hpp" -o -name "*.h" -o -name "*.cpp" -o -name "*.c" > cscope_my.files

cscope -bkq -i cscope_my.files  -f cscope_my.out

for i in $DIR/compat $DIR/lib $DIR/maint $DIR/tests $DIR/win32 
do 
	cp .vimrc	$i/
done

ctags * 

