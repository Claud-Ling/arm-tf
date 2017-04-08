
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern char *strGroups[];

static char str[12][16];

int
getgroup(const char *src, char *dest)
{
	int group_id;
	
	if(*src++ != '{')
	{
		return 0;
	}
	
	while(isdigit(*src) || isalpha(*src) || *src == '_')
	{
		*dest++ = *src++;
	}
	*dest = '\0';
	if(*src++ != ':')
	{
		return 0;
	}
	
	group_id = strtol(src, NULL, 0);
	if(0 < group_id || group_id <= 12)
	{
		return group_id;
	}
	
	return 0;
}

int
getgroups(const char *config_path)
{
	FILE *fp;
	char buff[1024];
	int ret = 0;
	int j;

	fp = fopen(config_path, "r");
	if(!fp)
	{
		fprintf(stderr, "Can't open %s\n", config_path);
		return 0;
	}

	while(fgets(buff, 1024, fp))
	{
		if(strncmp(buff, "[GROUPS]=", 9) == 0)
		{
			char *p = &buff[9];
			char *q;
			for(j = 0; j < 12; j++)
			{
				strGroups[j] = NULL;
			}
			while(p)
			{
				char tmp[16];
				int group_id;

				q = strchr(p, '}');
				if(!q)
					break;
				*q = '\0';
				if(0 == (group_id = getgroup(p, tmp)))
				{
					break;
				}
				strcpy(str[group_id], tmp);
				strGroups[group_id] = str[group_id];
				p = q + 1;
			}
			ret = 1;
			break;
		}
	}

	fclose(fp);

	return ret;
}

