
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TRACE(...)

typedef uint16_t small_grid_t[3][3];
typedef small_grid_t grid_t[3][3];

#define ANY 0x1FF
#define B(val) (1 << (val - 1))

uint16_t changed;

static uint16_t _at (grid_t grid, uint16_t x, uint16_t y)
{
  return grid[y/3][x/3][y%3][x%3];
}

static void _set (grid_t grid, uint16_t x, uint16_t y, uint16_t val)
{
  uint16_t before = grid[y/3][x/3][y%3][x%3];
  TRACE("[%u,%u] = ", x, y);

  uint16_t b;

  for (b = 8; b <= 8; b--) {
    if ((before & (1 << b)) == (val & (1 << b))) {
      TRACE("%u", (val >> b) & 1);
    } else {
      TRACE("[1m%u[m", (val >> b) & 1);      
    }
  }

  TRACE("\n");
  
  grid[y/3][x/3][y%3][x%3] = val;
  changed++;
}

static uint16_t _nbit (uint16_t val)
{
  uint16_t n = 0;

  while (val) {
    if (val & 1) {
      n++;
    }

    val = val >> 1;
  }

  return n;
}

static void _display_grid (grid_t grid, uint16_t x, uint16_t y)
{
  uint16_t i, j;

  for (j = 0; j < 9; j++) {
    for (i = 0; i < 9; i++) {
      uint16_t val = _at(grid, i, j);
      
      if (_nbit(val) > 1) {
	printf(" ");
      } else {
	uint16_t n = 1;

	while ((val&1) == 0) {
	  n++;
	  val = val >> 1;
	}

	if ((i == x) && (j == y)) {
	  printf("[5m%u[m", n);	  
	} else {
	  printf("%u", n);
	}
      }

      if ((i%3) == 2) {
	printf("|");
      } else {
	printf(" ");
      }
    }

    printf("\n");

    if ((j%3) == 2) {
      printf("------------------\n");
    }
  }

  TRACE("new: %d,%d\n", (int16_t)x, (int16_t)y);
}

uint16_t nx, ny;

uint16_t _reduce_line (grid_t grid, uint16_t y)
{
  uint16_t x;

  for (x = 0; x < 9; x++) {
    uint16_t val = _at(grid, x, y);

    if (_nbit(val) > 1) continue;

    uint16_t x2;
    
    for (x2 = 0; x2 < 9; x2++) {
      if (x2 != x) {
	uint16_t before = _at(grid, x2, y);
	uint16_t after = before & ~val;
	
	if (before != after) {
	  _set(grid, x2, y, after);

	  if (_nbit(after) == 1) {
	    nx = x2;
	    ny = y;
	    return 1;
	  }
	}
      }
    }
  }

  return 0;
}

uint16_t _reduce_column (grid_t grid, uint16_t x)
{
  uint16_t y;

  for (y = 0; y < 9; y++) {
    uint16_t val = _at(grid, x, y);

    if (_nbit(val) > 1) continue;

    uint16_t y2;
    
    for (y2 = 0; y2 < 9; y2++) {
      if (y2 != y) {
	uint16_t before = _at(grid, x, y2);
	uint16_t after = before & ~val;
	
	if (before != after) {
	  _set(grid, x, y2, after);

	  if (_nbit(after) == 1) {
	    nx = x;
	    ny = y2;
	    return 1;
	  }
	}
      }
    }
  }

  return 0;
}

uint16_t _reduce_small (grid_t grid, uint16_t si, uint16_t sj)
{
  uint16_t i, j;
  uint16_t i2, j2;
  
  for (j = 0; j < 3; j++) {
    for (i = 0; i < 3; i++) {
      uint16_t val = grid[sj][si][j][i];

      if (_nbit(val) == 1) {
	for (j2 = 0; j2 < 3; j2++) {
	  for (i2 = 0; i2 < 3; i2++) {
	    if ((j2 != j) && (i2 != i)) {
	      uint16_t before = grid[sj][si][j2][i2];

	      uint16_t after = before & ~val;

	      if (before != after) {
		_set(grid, si*3+i2, sj*3+j2, after);

		if (_nbit(after) == 1) {
		  nx = si*3+i2;
		  ny = sj*3+j2;
		  return 1;
		}
	      }
	    }
	  }
	}
      }		
    }
  }
  return 0;
}

uint16_t _reduce_small_line (grid_t grid, uint16_t si, uint16_t sj, uint16_t line, uint16_t val)
{
  uint16_t i;

  for (i = 0; i < 3; i++) {
    uint16_t before = grid[sj][si][line][i];
    uint16_t after = before & ~val;
	
    if (before != after) {
      _set(grid, si*3+i, sj*3+line, after);
      
      if (_nbit(after) == 1) {
	nx = si*3+i;
	ny = sj*3+line;
	return 1;
      }
    }
  }

  return 0;
}

uint16_t _reduce_small_column (grid_t grid, uint16_t si, uint16_t sj, uint16_t col, uint16_t val)
{
  uint16_t j;

  for (j = 0; j < 3; j++) {
    uint16_t before = grid[sj][si][j][col];
    uint16_t after = before & ~val;
	
    if (before != after) {
      _set(grid, si*3+col, sj*3+j, after);
      
      if (_nbit(after) == 1) {
	nx = si*3+col;
	ny = sj*3+j;
	return 1;
      }
    }    
  }

  return 0;
}

uint16_t _reduce_group_small (grid_t grid, uint16_t si, uint16_t sj)
{
  // reduce this kind of case:
  //        | X X X | 4 x x
  //        | 1 2 3 | x x x
  // (here) | X X X | x x x
  //    |
  //  cannot be 4
  uint16_t n;
  
  // for each value
  for (n = 1; n <= 9; n++) {
    uint16_t v = (1 << (n - 1));
    uint16_t c0, c1, c2, l0, l1, l2;

    // sub-line
    l0 = (grid[sj][si][0][0] & v) | (grid[sj][si][0][1] & v) | (grid[sj][si][0][2] & v);
    l1 = (grid[sj][si][1][0] & v) | (grid[sj][si][1][1] & v) | (grid[sj][si][1][2] & v);
    l2 = (grid[sj][si][2][0] & v) | (grid[sj][si][2][1] & v) | (grid[sj][si][2][2] & v);

    if (l0 && (! l1) && (!l2)) {
      // line 0 only
      if (_reduce_small_line(grid, (si+1)%3, sj, 0, v)) return 1;
      if (_reduce_small_line(grid, (si+2)%3, sj, 0, v)) return 1;
    }

    if (l1 && (! l0) && (!l2)) {
      // line 1 only
      if (_reduce_small_line(grid, (si+1)%3, sj, 1, v)) return 1;
      if (_reduce_small_line(grid, (si+2)%3, sj, 1, v)) return 1;
    }

    if (l2 && (! l0) && (!l1)) {
      // line 2 only
      if (_reduce_small_line(grid, (si+1)%3, sj, 2, v)) return 1;
      if (_reduce_small_line(grid, (si+2)%3, sj, 2, v)) return 1;
    }

    // sub-coloumn
    c0 = (grid[sj][si][0][0] & v) | (grid[sj][si][1][0] & v) | (grid[sj][si][2][0] & v);
    c1 = (grid[sj][si][0][1] & v) | (grid[sj][si][1][1] & v) | (grid[sj][si][2][1] & v);
    c2 = (grid[sj][si][0][2] & v) | (grid[sj][si][1][2] & v) | (grid[sj][si][2][2] & v);

    if (c0 && (! c1) && (!c2)) {
      // column 0 only
      if (_reduce_small_column(grid, si, (sj+1)%3, 0, v)) return 1;
      if (_reduce_small_column(grid, si, (sj+2)%3, 0, v)) return 1;
    }

    if (c1 && (! c0) && (!c2)) {
      // column 1 only
      if (_reduce_small_column(grid, si, (sj+1)%3, 1, v)) return 1;
      if (_reduce_small_column(grid, si, (sj+2)%3, 1, v)) return 1;
    }

    if (c2 && (! c0) && (!c1)) {
      // column 2 only
      if (_reduce_small_column(grid, si, (sj+1)%3, 2, v)) return 1;
      if (_reduce_small_column(grid, si, (sj+2)%3, 2, v)) return 1;
    }
  }

  return 0;
}

uint16_t _alone_small (grid_t grid, uint16_t si, uint16_t sj)
{
  uint16_t n;

  for (n = 1; n <= 9; n++) {
    uint16_t i, j;
    uint16_t occur = 0;
    uint16_t last_i, last_j;
    uint16_t v = (1 << (n - 1));
    
    for (j = 0; j < 3; j++) {
      for (i = 0; i < 3; i++) {
	
	uint16_t val = grid[sj][si][j][i];

	if (val == v) {
	  goto next_val;
	}

	if (val & v) {
	  occur++;
	  last_i = i;
	  last_j = j;
	}
      }
    }

    if (occur == 1) {
      _set(grid, si*3+last_i, sj*3+last_j, v);
      nx = si*3+last_i;
      ny = sj*3+last_j;
      return 1;      
    }

  next_val:
  }

  return 0;
}

// detect if a value is possible in a single position on a line
uint16_t _alone_line (grid_t grid, uint16_t line)
{
  uint16_t n;

  for (n = 1; n <= 9; n++) {
    uint16_t i;
    uint16_t occur = 0;
    uint16_t last_i;
    uint16_t v = (1 << (n - 1));
    
    for (i = 0; i < 9; i++) {
	
      uint16_t val = _at(grid, i, line);

      if (val == v) {
	goto next_val;
      }

      if (val & v) {
	occur++;
	last_i = i;
      }
    }

    if (occur == 1) {
      _set(grid, last_i, line, v);
      nx = last_i;
      ny = line;
      return 1;      
    }

  next_val:
  }

  return 0;
}

// detect if a value is possible in a single position on a colunm
uint16_t _alone_column (grid_t grid, uint16_t col)
{
  uint16_t n;

  for (n = 1; n <= 9; n++) {
    uint16_t j;
    uint16_t occur = 0;
    uint16_t last_j;
    uint16_t v = (1 << (n - 1));
    
    for (j = 0; j < 9; j++) {
	
      uint16_t val = _at(grid, col, j);

      if (val == v) {
	goto next_val;
      }

      if (val & v) {
	occur++;
	last_j = j;
      }
    }

    if (occur == 1) {
      _set(grid, col, last_j, v);
      nx = col;
      ny = last_j;
      return 1;      
    }

  next_val:
  }

  return 0;
}

uint16_t _read_value (FILE *pf)
{
  while (1) {
    int c = fgetc(pf);

    if ((c >= '1') && (c <= '9')) return B((1+c-'1'));
    if ((c == ' ') || (c == '.')) return ANY;
    if ((c != '\n') && (c != '\r') && (c != '-') && (c != '|')) {
      printf("bad char %u\n", c);
      return 0;
    }
  }
}

int main (int argc, char *argv[])
{
  FILE *pf;
  uint16_t step_mode = 0;
  uint32_t total_ops = 0;

  // check options
  argv++;
  argc--;
  
  while (argc && (argv[0][0] == '-')) {
    if (strcmp("-s", argv[0]) == 0) {
      step_mode = 1;
    } else if (strcmp("-h", argv[0]) == 0) {
      printf("sudoku [-s] [file]\n");
      printf("  -s  step mode\n");
      printf("  if no file is provided, uses stdin.\n");
      return 0;
    } else {
      printf("invalid option: %s\n", argv[0]);
      return -1;
    }

    argv++;
    argc--;
  }
  
  if (argc == 0) {
    pf = stdin;
  } else {
    pf = fopen(argv[0], "r");

    if (! pf) {
      printf("cannot open file\n");
      return -1;
    }
  }

  grid_t grid = { { { { ANY } } } };

  uint16_t line, col;

  for (line = 0; line < 9; line++) {
    for (col = 0; col < 9; col++) {
      uint16_t val = _read_value(pf);

      if (val == 0) {
	printf(" error reading value at location [%u,%u]\n", col+1, line+1);
	return -1;
      }

      grid[line/3][col/3][line%3][col%3] = val;
    }
  }

  /*
  grid_t grid = { { { { B(5), ANY, B(7) }, { B(2), B(6), ANY },  { ANY, ANY, ANY } },
		    { { ANY, B(9), ANY }, { ANY, ANY, B(4) }, { ANY, ANY, ANY } },
		    { { B(2), ANY, ANY }, { ANY, ANY, ANY }, { B(5), B(6), ANY } } },
	   
		  { { { ANY, ANY, ANY }, { ANY, B(9), ANY },  { B(8), ANY, ANY } },
		    { { ANY, ANY, ANY }, { B(3), B(2), B(6) }, { ANY, ANY, ANY } },
		    { { ANY, ANY, B(5) }, { ANY, B(4), ANY }, { ANY, ANY, ANY } } },
	   
		  { { { ANY, B(7), B(8) }, { ANY, ANY, ANY },  { ANY, ANY, B(4) } },
		    { { ANY, ANY, ANY }, { B(7), ANY, ANY }, { ANY, B(6), ANY } },
		    { { ANY, ANY, ANY }, { ANY, B(2), B(9) }, { B(8), ANY, B(3) } } } };
  */
  
  nx = -1;
  ny = -1;

  if (step_mode) {
    _display_grid(grid, nx, ny);
    getchar();
  }
  
  while (1) {
    
    changed = 0;

    uint16_t si, sj;
    uint16_t x, y;

    // reveale alone in small grid
    TRACE("step 1\n");
    
    for (sj = 0; sj < 3; sj++) {
      for (si = 0; si < 3; si++) {
	if (_alone_small(grid, si, sj)) goto end_loop;
      }
    }

    // check small grids
    TRACE("step 2\n");
    
    for (sj = 0; sj < 3; sj++) {
      for (si = 0; si < 3; si++) {
	if (_reduce_small(grid, si, sj)) goto end_loop;
      }
    }

    // reduce lines
    TRACE("step 3\n");
    
    for (y = 0; y < 9; y++) {
      if (_reduce_line(grid, y)) goto end_loop;
    }

    // reduce columns
    TRACE("step 4\n");
    
    for (x = 0; x < 9; x++) {
      if (_reduce_column(grid, x)) goto end_loop;
    }

    // single option on lines
    TRACE("step 5\n");
    
    for (y = 0; y < 9; y++) {
      if (_alone_line(grid, y)) goto end_loop;
    }

    // single option on columns
    TRACE("step 6\n");
    
    for (x = 0; x < 9; x++) {
      if (_alone_column(grid, x)) goto end_loop;
    }

    //
    TRACE("step 7\n");
    
    for (sj = 0; sj < 3; sj++) {
      for (si = 0; si < 3; si++) {
	if (_reduce_group_small(grid, si, sj)) goto end_loop;
      }
    }

  end_loop:

    total_ops += changed;
    
    if (! changed) {
      break;
    }
    
    if (step_mode) {
      _display_grid(grid, nx, ny);
      getchar();
    }
    
    nx = -1;
    ny = -1;
  }

  if (! step_mode) {
    _display_grid(grid, -1, -1);
  }
  
  printf("# ops: %u\n", total_ops);

  return 0;
}
