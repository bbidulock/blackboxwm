#include <stdlib.h>
#include <stdio.h>

#define NUMBER_OF_RUNS 10000

static char * strings_base[NUMBER_OF_RUNS], ** strings = strings_base;

static void h(int r)
{
  if(r<0) r = -r;
  *strings++ = (char*) malloc(r + 10);
}

static void g(int r)
{
  int s;

  switch((s = (rand() % 10)))
    {
      default:
      case 0:	h(r + s); break;
      case 1:	h(r + s); break;
      case 2:	h(r + s); break;
      case 3:	h(r + s); break;
      case 4:	h(r + s); break;
      case 5:	h(r + s); break;
      case 6:	h(r + s); break;
      case 7:	h(r + s); break;
      case 8:	h(r + s); break;
      case 9:	h(r + s); break;
    }
}

static void f(int r)
{
  int s;

  switch((s = (rand() % 10)))
    {
      default:
      case 0:	g(r + s); break;
      case 1:	g(r + s); break;
      case 2:	g(r + s); break;
      case 3:	g(r + s); break;
      case 4:	g(r + s); break;
      case 5:	g(r + s); break;
      case 6:	g(r + s); break;
      case 7:	f(r + s); break;
      case 8:	g(r + s); break;
      case 9:	g(r + s); break;
    }
}

void main()
{
  int r, i;

  srand(47110815);

  printf("  allocating   .....");

  for(i=0; i<NUMBER_OF_RUNS; i++)
    {
      printf("%5d", i);
      fflush(stdout);

      switch((r = (rand() % 100)))
        {
          default:
          case 0:	f(r); break;
          case 1:	f(r); break;
          case 2:	f(r); break;
          case 3:	f(r); break;
          case 4:	f(r); break;
          case 5:	f(r); break;
          case 6:	f(r); break;
          case 7:	f(r); break;
          case 8:	f(r); break;
          case 9:	f(r); break;
        }
    }

  printf("\n  deallocating .....");

  for(i=0; i<NUMBER_OF_RUNS; i++)
    {
      printf("%5d", i);
      fflush(stdout);

      switch(rand() % (NUMBER_OF_RUNS/10))
        {
	  case 0:
	    break;			/* produce garbage */
	  
	  case 1:			/* double free */
	    free(strings_base[i]);
	    free(strings_base[i]);
	    break;
	  
	  default:
	    free(strings_base[i]);	/* ok ! */
	    break;
	}
    }    

  printf("\n");
  fflush(stdout);
}
