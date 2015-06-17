#include "bsp.h"

void Eatline(FILE *fp)
{
	while(!feof(fp))
	{
		if(fgetc(fp) == '\n')
			break;
	}
}

// note: returns a pointer to a static buffer
char* ReadToken(FILE *fp)
{
	static char buffer[1024];

	int ret = fscanf(fp, "%s", buffer);

	if(ret == 0 || ret == EOF)
	{
		return NULL;
	}

	return buffer;
}

// destructively modifies the string in place
char* StripQuotes(char *string)
{
	char *src = string;
	char *dst = string;

	while(*src)
	{
		if(*src == '\"')
		{
			src++;
			continue;
		}

		*dst++ = *src++;
	}

	*dst = '\0';

	return string;
}

// note: returns a pointer to a static buffer
char* ReadQuotedString(FILE *fp)
{
	static char buffer[1024];
	fscanf(fp, "%s", buffer);

	StripQuotes(buffer);

	return buffer;
}


