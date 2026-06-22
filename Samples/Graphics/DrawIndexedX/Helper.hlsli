
// The thread mask stores a bit for each thread.
// .x stores values for threads 1 to 32 and .y stores 33 to 64
// Get the first active thread in the entire wave. This is used to 
// increment the counter only once per wave.
uint GetFirstActiveThread(uint2 threadMask)
{
	uint firstThread = 0;
	if (threadMask.x != 0)
	{
		firstThread = firstbitlow(threadMask.x);
	}
	else
	{
		firstThread = firstbitlow(threadMask.y) + 32;
	}

	return firstThread;
}