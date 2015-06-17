#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKEN_SIZE	1024

static char **src;
static char token[MAX_TOKEN_SIZE];
static int numchars;

static int CurrentChar()
{
	return **src;
}

static void DiscardChar()
{
	(*src)++;
}

static void AcceptChar()
{
	if(numchars >= MAX_TOKEN_SIZE - 2)
		return;

	token[numchars] = CurrentChar();
	numchars++;

	DiscardChar();
}

static void Accept();
static void Error();
static void EndToken();
static void Token1();
static void Token2();
static void Quote1();
static void Quote2();

enum state_t
{
	S_ERROR,
	S_ACCEPT,
	S_ENDTOKEN,
	S_TOKEN1,
	S_TOKEN2,
	S_QUOTE1,
	S_QUOTE2
};

static void (*statetable[])() =
{
	NULL,
	NULL,
	EndToken,
	Token1,
	Token2,
	Quote1,
	Quote2
};

static state_t	state;

static void SetState(state_t newstate)
{
	state = newstate;
}

static void EndToken()
{
	token[numchars] = 0;

	SetState(S_ACCEPT);
}

static void Token1()
{
	int c = CurrentChar();

	if(!c)
	{
		SetState(S_ERROR);
		return;
	}

	if(isalnum(c))
	{
		AcceptChar();
		SetState(S_TOKEN2);
		return;
	}

	DiscardChar();
}

static void Token2()
{
	int c  = CurrentChar();

	if(!c || !isalnum(c))
	{
		SetState(S_ENDTOKEN);
		return;
	}

	AcceptChar();
}

static void Quote1()
{
	int c = CurrentChar();

	if(!c)
	{
		SetState(S_ERROR);
		return;
	}

	if(c == '"')
	{
		DiscardChar();
		SetState(S_QUOTE2);
		return;
	}

	DiscardChar();
}

static void Quote2()
{
	int c = CurrentChar();

	if(!c)
	{
		SetState(S_ERROR);
		return;
	}

	if(c == '"')
	{
		DiscardChar();
		SetState(S_ENDTOKEN);
		return;
	}

	AcceptChar();
}

static char *RunStateMachine(char **string, state_t startstate)
{
	src = string;
	numchars = 0;
	state = startstate;

	while(state != S_ACCEPT && state != S_ERROR)
	{
		statetable[state]();
	}

	if(state == S_ERROR)
		return NULL;

	return token;
}

char *GetToken(char **string)
{
	return RunStateMachine(string, S_TOKEN1);
}

char *GetQuotedString(char **string)
{
	return RunStateMachine(string, S_QUOTE1);
}

