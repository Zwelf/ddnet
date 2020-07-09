#ifndef ENGINE_SERVER_SQL_STRING_HELPERS_H
#define ENGINE_SERVER_SQL_STRING_HELPERS_H

namespace sqlstr
{

void FuzzyString(char *pString, int size);

// written number of added bytes
int EscapeLike(char *pDst, const char *pSrc, int DstSize);

void AgoTimeToString(int agoTime, char *pAgoString);

}

#endif
