source_file="Null"
clean_files="Null"
object_file="Null"
malloc_flag="Null"
null="Null"

# Input flags
while getopts c:r:o:m: flag
do
	case "${flag}" in
		o) object_file=${OPTARG};;
		c) source_file=${OPTARG};;
		m) malloc_flag=${OPTARG};;
		r) clean_files=${OPTARG};;
	esac
done

if [[ $clean_files == "all" ]]; then
	rm -f *.o
	rm -f *.riscv
	echo ">>Jessica:  Removing generated files ";
fi


if [[ $source_file != $null ]]; then
	echo ">>Jessica:  Source FIle: $source_file.c";
	echo ">>Jessica:  ============================================ ";

	if [ -f "$source_file.o" ]; then
		rm $source_file.o
		echo ">>Jessica:  Removing $source_file.o ";
	fi

	if [ -f "$source_file.riscv" ]; then
		rm $source_file.riscv
		echo ">>Jessica:  Removing $source_file.riscv ";
	fi

	riscv64-unknown-elf-gcc -fno-common -fno-builtin-printf -specs=htif_nano.specs -march=rv64imafd -O3 -c $source_file.c tasks.c deque.c
	
	if [[ $malloc_flag != $null ]]; then
		riscv64-unknown-elf-gcc -fno-common -fno-builtin-printf -specs=htif_nano.specs -march=rv64imafd -static -Wl,--allow-multiple-definition -DUSE_PUBLIC_MALLOC_WRAPPERS ./malloc.o $source_file.o ./tasks.o ./deque.o -O3 -o $source_file.riscv
	fi

	if [[ $malloc_flag == $null ]]; then
		riscv64-unknown-elf-gcc -fno-common -fno-builtin-printf -specs=htif_nano.specs -march=rv64imafd $source_file.o ./tasks.o ./deque.o -O3 -o $source_file.riscv
	fi
	
	echo ">>Jessica:  Generating $source_file.riscv ";
fi

if [ $object_file != $null ]; then
	echo ">>Jessica:  Source FIle: $object_file.c";
	echo ">>Jessica:  ============================================ ";

	if [ -f "$object_file.o" ]; then
		rm $object_file.o
		echo ">>Jessica:  Removing $object_file.o ";
	fi

	riscv64-unknown-elf-gcc -fno-common -fno-builtin-printf -specs=htif_nano.specs -march=rv64imafd -DUSE_PUBLIC_MALLOC_WRAPPERS -O3 -c $object_file.c 
	echo ">>Jessica:  Generating $object_file.o ";
fi
