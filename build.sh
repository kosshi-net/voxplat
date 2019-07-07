for f in ./resources/*.glsl; do
	if ! glslangValidator $f; then
		exit 1;
	fi
done

cd build;
rm src/resources.o;
make -j12 && src/game $@;

