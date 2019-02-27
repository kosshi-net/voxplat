// deadcode.c
// File for archiving away stuff maybe useful in the future


uint32_t chunkset_mesher_dumb(
	struct ChunkSet *set,
	struct ChunkMD *c,
	int16_t *buf
){

	uint32_t v = 0; // vertex count
	
	uint16_t vec[3];
	
	for( vec[2] = 0; vec[2] < set->root; vec[2]++  )
	for( vec[1] = 0; vec[1] < set->root; vec[1]++  )
	for( vec[0] = 0; vec[0] < set->root; vec[0]++  )
	{
		uint32_t i = flatten1(vec, set->root_bitw);
		Voxel vxl = c->voxels[i];
		if( vxl == 0 ) continue;

		if( !voxel_visible( set, c, vec, i  ) ) continue;

		int32_t ws[3];
		for( int j = 0; j < 3; j++  )
			ws[j] = set->root * c->offset[j] + vec[j] ;

		buf[v++] = -ws[0];
		buf[v++] = -ws[1];
		buf[v++] = -ws[2];
		buf[v++] = vxl&0x003F;
		
		//_work_buf[id][v++] = ( vxl >> 16 &0xFF ) / 256.0f;
		//_work_buf[id][v++] = ( vxl >> 8  &  0xFF ) / 256.0f;
		//_work_buf[id][v++] = ( vxl&0xFF ) / 256.0f;

	}

	return v;
}


uint32_t chunkset_mesher_simd(
	struct ChunkSet *set,
	struct ChunkMD *c,
	int16_t *geometry,
	uint32_t *mask
){
		

	uint16_t cvec[3];
	memcpy( cvec, c->offset, sizeof(cvec) ); 

	//struct ChunkMD *nc[3];
	Voxel *vox[3];
	Voxel *voxels = c->voxels;

	for( int i = 0; i < 3; i++ ) {
		cvec[i]++;
		if( cvec[i]+1 > set->max[i]  )
			vox[i] = set->null_chunk->voxels;
		else{
			vox[i] = set->chunks[ flatten3( cvec, set->max_bitw ) ].voxels;
			chunk_touch_ro(&set->chunks[ flatten3( cvec, set->max_bitw ) ]);
		}
		cvec[i]--;
	}

	uint16_t r = set->root-1;
	uint16_t AC[3];
	uint32_t Ai = 0;
	int batch = 32;
	for( AC[2] = 0; AC[2] < set->root-1; AC[2]++ )
	for( AC[1] = 0; AC[1] < set->root-1; AC[1]++ )
	for( AC[0] = 0; AC[0] < set->root-1; AC[0]+=batch )
	{
		Ai = flatten1( AC, set->root_bitw  );
		float A[batch];

		#pragma omp simd
		for( int s = 0; s < batch; s++ )
			A[s] = !voxels[Ai+s];

		uint32_t shift = 0;
		for( int i = 0; i < 3; i++ ){
			float B[batch];

			uint32_t Bi = (Ai + (1 << shift));

			#pragma omp simd
			for( int s = 0; s < batch; s++ ){
				B[s] = !voxels[ Bi + s ];
			}

			shift += set->root_bitw;

			float R[batch];

			#pragma omp simd
			for( int s = 0; s < batch; s++ ){
				R[s] = A[s] == B[s];
			}
			for( int s = 0; s < batch; s++ ){
				if ( R[s] ) continue; // yeet
				uint16_t MC[3];
				memcpy( MC, AC, sizeof(MC) );
				MC[0]+=s;
				MC[i] += (A[s]==0);
				uint32_t Mi = flatten1( MC, set->root_bitw+1 );
				mask[Mi] = A[s]+B[s]; // TODO Make this into a proper bit mask thing
			}
		}
		//Ai+=8;
	}

	uint32_t v = 0;
	// Loop thru the mask
	for( AC[2] = 0; AC[2] < set->root+1; AC[2]++ )
	for( AC[1] = 0; AC[1] < set->root+1; AC[1]++ )
	for( AC[0] = 0; AC[0] < set->root+1; AC[0]++ )
	{
		uint32_t Mi = flatten1( AC, set->root_bitw+1 );
		if( mask[Mi] == 0 ) continue;

		for( int j = 0; j < 3; j++  )
			geometry[v++] = -((c->offset[j] << set->root_bitw) + AC[j]);

		geometry[v++] = mask[Mi];
		mask[Mi] = 0;
	}

	return v;
}


