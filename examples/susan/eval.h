double score_me(unsigned char *original, unsigned char *relaxed, int width, int height) 
{
	int incorrects = 0;
	int total = 0;

	for(int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			if (original[i*width + j] != relaxed[i*width + j]) {
            	incorrects += 1;
            }
        	total += 1;
		}
	}
    
    return (double)incorrects / (double)total;
}