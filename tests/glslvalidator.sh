for f in ./resources/*.glsl; do
	echo $f;
	if ! glslangValidator $f; then
		exit 1;
	fi

done