
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct s_arena
{
	void			*buf;
	size_t			buf_size;
	size_t			prev_offset;
	size_t			curr_offset;
}					t_arena;

typedef struct
{
	char			**board;
	int				size;
}					t_board;

typedef struct
{
	t_board			*board;
	int				player_turn;
	int				game_type;
	pthread_mutex_t	*mutex;
	sem_t			*sem;
	pthread_t		thread[2];
	t_arena			*arena;
	int				done;
	int				player1;
}					t_game;

void				*arena_init(size_t buffer_size);
void				arena_reset(t_arena *a);
void				*arena_alloc(t_arena *a, size_t size);
void				arena_destroy(t_arena *a);
void				set_game_mode(t_arena *a, t_game *game);
void				print_board(t_board *board);
void				set_boardsize(t_arena *arena, t_board *board);
void				print_board(t_board *board);
void				init_board(t_arena *arena, t_board *board);
void				is_game_done(t_arena *arena, t_board *board, t_game *game);
int					verifierMatchNul(char **plateau, int boardSize);
int					verifierGagnantDynamic(char **plateau, char symbole,
						int boardSize);
void				play_one_turn(t_arena *arena, t_board *board, t_game *game);
void				tourOrdinateur(t_board *board, t_game *game);
void				init_game(t_arena *arena, t_game *game);
void				*thread_IA1(void *arg);
void				*thread_IA2(void *arg);

static void	*arena_memset(void *s, int c, size_t n)
{
	size_t	i;

	i = -1;
	while (++i < n)
		((unsigned char *)s)[i] = c;
	return (s);
}

static int	is_power_of_two(uintptr_t x)
{
	return ((x & (x - 1)) == 0);
}

static uintptr_t	align_forward(uintptr_t ptr, size_t align)
{
	uintptr_t	p;
	uintptr_t	a;
	uintptr_t	modulo;

	if (!is_power_of_two(align))
		exit(1);
	p = ptr;
	a = (uintptr_t)align;
	modulo = p % a;
	if (modulo != 0)
		p += a - modulo;
	return (p);
}

void	*arena_alloc(t_arena *a, size_t size)
{
	uintptr_t	curr_ptr;
	uintptr_t	offset;
	void		*ptr;

	curr_ptr = (uintptr_t)a->buf + (uintptr_t)a->curr_offset;
	offset = align_forward(curr_ptr, sizeof(void *));
	offset -= (uintptr_t)a->buf;
	if (offset + size > a->buf_size)
		return (NULL);
	ptr = &((unsigned char *)a->buf)[offset];
	a->prev_offset = offset;
	a->curr_offset = offset + size;
	arena_memset(ptr, 0, size);
	return (ptr);
}

void	*arena_init(size_t buffer_size)
{
	t_arena	*a;
	void	*buf;

	buf = malloc(buffer_size);
	if (!buf)
		return (NULL);
	a = malloc(sizeof(t_arena));
	if (!a)
		return (NULL);
	a->buf = buf;
	a->buf_size = buffer_size;
	a->curr_offset = 0;
	a->prev_offset = 0;
	return (a);
}

void	arena_reset(t_arena *a)
{
	a->curr_offset = 0;
	a->prev_offset = 0;
}

void	arena_destroy(t_arena *a)
{
	arena_reset(a);
	if (a->buf)
		free(a->buf);
	free(a);
}

void	init_board(t_arena *arena, t_board *board)
{
	int	i;
	int	j;

	i = 0;
	board->board = (char **)arena_alloc(arena, sizeof(char *) * board->size);
	while (i < board->size)
	{
		j = 0;
		while (j < board->size)
		{
			if (j == 0)
				board->board[i] = (char *)arena_alloc(arena, sizeof(char)
					* board->size);
			board->board[i][j] = ' ';
			j++;
		}
		i++;
	}
}

void	print_board(t_board *board)
{
	int	i;
	int	j;

	i = 0;
	while (i < board->size)
	{
		j = 0;
		while (j < board->size)
		{
			putchar(' ');
			putchar(board->board[i][j]);
			putchar(' ');
			if (j < board->size - 1)
				putchar('|');
			j++;
		}
		putchar('\n');
		if (i < board->size - 1)
		{
			j = 0;
			while (j < board->size)
			{
				printf("---");
				if (j < board->size - 1)
					putchar('+');
				j++;
			}
			putchar('\n');
		}
		i++;
	}
	putchar('\n');
}

void	set_game_mode(t_arena *arena, t_game *game)
{
	char	*gt;
	int		game_type;
	int		nb;

	game_type = 0;
	gt = (char *)arena_alloc(arena, sizeof(char) * 2);
	while (game_type != 1 && game_type != 2 && game_type != 3 && game_type != 4)
	{
		printf("Choose game mode:\n");
		printf("1. Player vs Player\n");
		printf("2. Player vs AI\n");
		printf("3. AI vs AI\n");
		printf("4. Uploading a save\n");
		nb = scanf("%s", gt);
		if (nb == 1)
			game_type = atoi(gt);
	}
	game->game_type = game_type;
	if (game_type == 1)
	{
		game->player_turn = 0;
		printf("Player 1 is X\n");
		printf("Player 2 is O\n");
	}
	else if (game_type == 2)
	{
		game->player_turn = 0;
		printf("Player is X\n");
		printf("AI is O\n");
	}
	else if (game_type == 3)
	{
		game->player_turn = 0;
		printf("AI 1 is X\n");
		printf("AI 2 is O\n");
	}
	else if (game_type == 4)
	{
		game->player_turn = 0;
	}
}

void	set_boardsize(t_arena *arena, t_board *board)
{
	int		size;
	char	*s;
	int		nb;

	size = 0;
	s = (char *)arena_alloc(arena, sizeof(char) * 2);
	while (size < 3 || size > 9)
	{
		printf("Choose board size (3-9):\n");
		nb = scanf("%s", s);
		if (nb == 1)
			size = atoi(s);
	}
	board->size = size;
}

void	tourOrdinateur(t_board *board, t_game *game)
{
	uintptr_t	game_ptr_val;
	FILE		*fp;
	char		filename[100];

	is_game_done(game->arena, board, game);
	int ligne, colonne;
	game_ptr_val = (uintptr_t)game;
	// Open the file for appending
	sprintf(filename, "game_coordinates_%lu.txt", game_ptr_val);
	fp = fopen(filename, "a");
	if (fp == NULL)
	{
		fprintf(stderr, "Failed to open file for writing\n");
		exit(EXIT_FAILURE);
	}
	srand(time(NULL) + game_ptr_val);
	while (1)
	{
		ligne = rand() % board->size;
		colonne = rand() % board->size;
		if (board->board[ligne][colonne] == ' ')
		{
			board->board[ligne][colonne] = game->player_turn == 0 ? 'X' : 'O';
			// Write the coordinates and player to the file
			fprintf(fp, "Player %d: (%d, %d)\n", game->player_turn + 1, ligne,
				colonne);
			break ;
		}
	}
	// Close the file
	fclose(fp);
}

void	play_one_turn(t_arena *arena, t_board *board, t_game *game)
{
	int			x;
	int			y;
	char		*input;
	int			nb;
	uintptr_t	game_ptr_val;
	FILE		*fp;
	char		filename[100];

	game_ptr_val = (uintptr_t)game;
	sprintf(filename, "game_coordinates_%lu.txt", game_ptr_val);
	fp = fopen(filename, "a");
	if (fp == NULL)
	{
		fprintf(stderr, "Failed to open file for writing\n");
		exit(EXIT_FAILURE);
	}
	x = 0;
	y = 0;
	input = (char *)arena_alloc(arena, sizeof(char) * 2);
	if (game->game_type == 1)
	{
		printf("Player %d's turn\n", game->player_turn + 1);
		printf("Enter coordinates (l c):\n");
		nb = scanf("%s", input);
		if (nb)
			x = atoi(input);
		nb = scanf("%s", input);
		y = atoi(input);
		while (x < 1 || x > board->size || y < 1 || y > board->size
			|| board->board[x - 1][y - 1] != ' ')
		{
			printf("Invalid coordinates\n");
			printf("Enter coordinates (l c):\n");
			nb = scanf("%s", input);
			if (nb)
				x = atoi(input);
			nb = scanf("%s", input);
			if (nb)
				y = atoi(input);
		}
		fprintf(fp, "Player %d: (%d, %d)\n", game->player_turn + 1, x - 1, y
			- 1);
		board->board[x - 1][y - 1] = game->player_turn == 0 ? 'X' : 'O';
		is_game_done(arena, board, game);
	}
	else if (game->game_type == 2)
	{
		if (game->player_turn == 0)
		{
			printf("Player's turn\n");
			printf("Enter coordinates (l c):\n");
			nb = scanf("%s", input);
			if (nb)
				x = atoi(input);
			nb = scanf("%s", input);
			if (nb)
				y = atoi(input);
			while (x < 1 || x > board->size || y < 1 || y > board->size
				|| board->board[x - 1][y - 1] != ' ')
			{
				printf("Invalid coordinates\n");
				printf("Enter coordinates (l c):\n");
				nb = scanf("%s", input);
				if (nb)
					x = atoi(input);
				nb = scanf("%s", input);
				if (nb)
					y = atoi(input);
			}
			fprintf(fp, "Player %d: (%d, %d)\n", game->player_turn + 1, x - 1, y
				- 1);
			board->board[x - 1][y - 1] = 'X';
			is_game_done(arena, board, game);
		}
		else
		{
			is_game_done(arena, board, game);
			printf("AI's turn\n");
			tourOrdinateur(board, game);
		}
	}
	game->player_turn = game->player_turn == 0 ? 1 : 0;
	fclose(fp);
}

int	verifierGagnantDynamic(char **plateau, char symbole, int boardSize)
{
	int	sequenceToWin;
	int	horizontalSequence;
	int	diagonalSequence1;
	int	diagonalSequence2;
	int	verticalSequence;

	sequenceToWin = 4;
	if (boardSize == 3)
		sequenceToWin = 3;
	for (int i = 0; i < boardSize; i++)
	{
		for (int j = 0; j < boardSize; j++)
		{
			if (plateau[i][j] == symbole)
			{
				horizontalSequence = 0, verticalSequence = 0;
				diagonalSequence1 = 0, diagonalSequence2 = 0;
				for (int k = 0; k < sequenceToWin; k++)
				{
					// Check boundaries before accessing array
					if (i + k < boardSize && plateau[i + k][j] == symbole)
						horizontalSequence++;
					if (j + k < boardSize && plateau[i][j + k] == symbole)
						verticalSequence++;
					if (i + k < boardSize && j + k < boardSize && plateau[i
						+ k][j + k] == symbole)
						diagonalSequence1++;
					if (i + k < boardSize && j - k >= 0 && plateau[i + k][j
						- k] == symbole)
						diagonalSequence2++;
				}
				// If any sequence reaches the needed sequenceToWin, return 1
				if (horizontalSequence == sequenceToWin
					|| verticalSequence == sequenceToWin
					|| diagonalSequence1 == sequenceToWin
					|| diagonalSequence2 == sequenceToWin)
					return (1);
			}
		}
	}
	return (0);
}

int	verifierMatchNul(char **plateau, int boardSize)
{
	int	i;
	int	j;

	i = 0;
	while (i < boardSize)
	{
		j = 0;
		while (j < boardSize)
		{
			if (plateau[i][j] == ' ')
				return (0);
			j++;
		}
		i++;
	}
	return (1);
}

void	is_game_done(t_arena *arena, t_board *board, t_game *game)
{
	char		symbole;
	uintptr_t	game_ptr_val;
	FILE		*fp;
	char		filename[100];

	game_ptr_val = (uintptr_t)game;
	sprintf(filename, "game_coordinates_%lu.txt", game_ptr_val);
	fp = fopen(filename, "a");
	if (fp == NULL)
	{
		fprintf(stderr, "Failed to open file for writing\n");
		exit(EXIT_FAILURE);
	}
	symbole = game->player_turn == 0 ? 'X' : 'O';
	if (game->player1 == 1)
		symbole = symbole == 'X' ? 'O' : 'X';
	if (verifierGagnantDynamic(board->board, symbole, board->size))
	{
		
		if (game->game_type != 3)
			printf("Player %d wins!\n", game->player_turn + 1);
		fprintf(fp, "Player %d wins\n", game->player_turn + 1);
		fclose(fp);
		if (game->game_type != 3)
		{
			print_board(board);
			arena_destroy(arena);
			exit(0);
		}
		game->player_turn = -1;
		return ;
	}
	symbole = game->player_turn == 1 ? 'O' : 'X';
	if (game->player1 == 1)
		symbole = symbole == 'X' ? 'O' : 'X';
	if (verifierGagnantDynamic(board->board, symbole, board->size))
	{
		if (game->game_type != 3)
			printf("Player %d wins!\n", game->player_turn + 1);
		fprintf(fp, "Player %d wins\n", game->player_turn + 1);
		fclose(fp);
		if (game->game_type != 3)
		{
			print_board(board);
			arena_destroy(arena);
			exit(0);
		}
		game->player_turn = -1;
		return ;
	}
	if (verifierMatchNul(board->board, board->size))
	{
		if (game->game_type != 3)
			printf("It's a tie!\n");
		fprintf(fp, "Tie\n");
		fclose(fp);
		if (game->game_type != 3)
		{
			print_board(board);
			arena_destroy(arena);
			exit(0);
		}
		game->player_turn = -1;
		return ;
	}
}

void	init_game(t_arena *arena, t_game *game)
{
	game->board = (t_board *)arena_alloc(arena, sizeof(t_board));
	game->player_turn = 0;
	if (game->game_type > 0 && game->game_type < 3)
		game->player_turn = game->game_type;
	else
		game->game_type = 0;
	game->mutex = (pthread_mutex_t *)arena_alloc(arena,
		sizeof(pthread_mutex_t));
	pthread_mutex_init(game->mutex, NULL);
	game->sem = (sem_t *)arena_alloc(arena, 2 * sizeof(sem_t));
	sem_init(game->sem, 0, 1);
	sem_init(game->sem + 1, 0, 0);
}

void	iavsiathread(t_arena *arena, int size)
{
	char	*input;
	int		nb;
	int		nbGames;
	int		i;
	t_board	*iaboard;
	t_game	*iagame;
	clock_t start, end;

	printf(" AI vs AI\n");
	printf(" How many games do you want to play? :\n");
	input = (char *)arena_alloc(arena, sizeof(char) * 2);
	nb = scanf("%s", input);
	nbGames = atoi(input);
	while (nbGames < 1 || nb != 1)
	{
		printf("Invalid number of games\n");
		printf("How many games do you want to play? :\n");
		nb = scanf("%s", input);
		if (nb)
			nbGames = atoi(input);
	}
	i = 0;
	start = clock();
	while (i < nbGames)
	{
		iagame = (t_game *)arena_alloc(arena, sizeof(t_game));
		init_game(arena, iagame);
		iaboard = iagame->board;
		iaboard->size = size;
		iagame->player_turn = 0;
		iagame->game_type = 3;
		iagame->board = iaboard;
		iagame->arena = arena;
		iagame->done = 0;
		init_board(arena, iaboard);
		pthread_create(iagame->thread, NULL, thread_IA1, (void *)iagame);
		pthread_create(iagame->thread + 1, NULL, thread_IA2, (void *)iagame);
		pthread_join(iagame->thread[0], NULL);
		pthread_join(iagame->thread[1], NULL);
		sem_destroy(iagame->sem);
		sem_destroy(iagame->sem + 1);
		pthread_mutex_destroy(iagame->mutex);
		i++;
	}
	end = clock();
	printf("Time taken: %f\n", ((double)(end - start)) / CLOCKS_PER_SEC);
	arena_destroy(arena);
	exit(0);
}

void	*thread_IA1(void *arg)
{
	t_game		*game;
	t_board		*board;
	t_arena		*arena;
	FILE		*fp;
	char		filename[100];
	uintptr_t	game_ptr_val;

	game = (t_game *)arg;
	board = game->board;
	arena = game->arena;
	int ligne, colonne;
	game = (t_game *)arg;
	board = game->board;
	arena = game->arena;
	game_ptr_val = (uintptr_t)game;
	sprintf(filename, "game_coordinates_%lu.txt", game_ptr_val);
	srand(time(NULL) + game_ptr_val);
	fp = fopen(filename, "a");
	if (fp == NULL)
	{
		fprintf(stderr, "Failed to open file for writing\n");
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "size:%d\n", game->board->size);
	fclose(fp);
	while (game->done != 1)
	{
		sem_wait(game->sem);
		if (game->done == 1)
			break ;
		pthread_mutex_lock(game->mutex);
		while (1)
		{
			ligne = rand() % board->size;
			colonne = rand() % board->size;
			if (board->board[ligne][colonne] == ' ')
			{
				board->board[ligne][colonne] = 'X';
				fp = fopen(filename, "a");
				if (fp == NULL)
				{
					fprintf(stderr, "Failed to open file for writing\n");
					exit(EXIT_FAILURE);
				}
				fprintf(fp, "Player 1: (%d, %d)\n", ligne, colonne);
				fclose(fp);
				break ;
			}
		}
		is_game_done(arena, board, game);
		if (game->player_turn == -1)
			game->done = 1;
		game->player_turn = 1;
		pthread_mutex_unlock(game->mutex);
		sem_post(game->sem + 1);
	}
	pthread_exit(NULL);
	return (NULL);
}

void	*thread_IA2(void *arg)
{
	t_game		*game;
	t_board		*board;
	t_arena		*arena;
	uintptr_t	game_ptr_val;
	FILE		*fp;
	char		filename[100];

	int ligne, colonne;
	game = (t_game *)arg;
	board = game->board;
	arena = game->arena;
	game_ptr_val = (uintptr_t)game;
	sprintf(filename, "game_coordinates_%lu.txt", game_ptr_val);
	srand(time(NULL) + game_ptr_val);
	while (game->done != 1)
	{
		sem_wait(game->sem + 1);
		if (game->done == 1)
			break ;
		pthread_mutex_lock(game->mutex);
		while (1)
		{
			ligne = rand() % board->size;
			colonne = rand() % board->size;
			if (board->board[ligne][colonne] == ' ')
			{
				board->board[ligne][colonne] = 'O';
				fp = fopen(filename, "a");
				if (fp == NULL)
				{
					fprintf(stderr, "Failed to open file for writing\n");
					exit(EXIT_FAILURE);
				}
				fprintf(fp, "Player 2: (%d, %d)\n", ligne, colonne);
				fclose(fp);
				break ;
			}
		}
		is_game_done(arena, board, game);
		if (game->player_turn == -1)
			game->done = 1;
		game->player_turn = 0;
		pthread_mutex_unlock(game->mutex);
		sem_post(game->sem);
	}
	pthread_exit(NULL);
	return (NULL);
}

void	printgame(t_arena *arena)
{
	char	filename[100];
	char	line[256];
	FILE	*fp;
	int		size;
	char	player;
	t_board	*board;
	int		turnbyturn;

	int x, y;
	board = (t_board *)arena_alloc(arena, sizeof(t_board));
	printf("Enter the game file name: ");
	scanf("%s", filename);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Failed to open file\n");
		exit(EXIT_FAILURE);
	}
	if (fscanf(fp, "size:%d\n", &size) != 1)
	{
		fprintf(stderr, "Failed to read size from file\n");
		exit(EXIT_FAILURE);
	}
	board->size = size;
	init_board(arena, board);
	printf("Do you want to see the game turn by turn? (y or n): ");
	scanf("%s", line);
	turnbyturn = line[0] == 'y' ? 1 : 0;
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		if (sscanf(line, "Player %c: (%d, %d)", &player, &x, &y) == 3)
		{
			if (turnbyturn)
				print_board(board);
			board->board[x][y] = player == '1' ? 'X' : 'O';
		}
		else
			printf("%s", line);
	}
	print_board(board);
	fclose(fp);
}

int	main(void)
{
	t_arena	*arena;
	t_board	*board;
	t_game	*game;
	char	*input;
	int		nb;
	//	minimum size of the arena for a 3x3 board is 512
	arena = arena_init(2147483647);
	board = (t_board *)arena_alloc(arena, sizeof(t_board));
	game = (t_game *)arena_alloc(arena, sizeof(t_game));
	set_boardsize(arena, board);
	init_board(arena, board);
	set_game_mode(arena, game);
	if (game->game_type == 4)
	{
		printgame(arena);
		arena_destroy(arena);
		return (0);
	}
	else if (game->game_type == 3)
	{
		init_game(arena, game);
		print_board(board);
		iavsiathread(arena, board->size);
		arena_destroy(arena);
		return (0);
	}
	else if (game->game_type == 2)
	{
		printf("Choose if you play first (y or n):\n");
		input = (char *)arena_alloc(arena, sizeof(char) * 2);
		nb = scanf("%s", input);
		if (nb == 1 && input[0] == 'y')
			game->player_turn = 0;
		else
		{
			game->player_turn = 1;
			game->player1 = 1;
		}
		print_board(board);
		while (1)
		{
			play_one_turn(arena, board, game);
			print_board(board);
		}
		arena_destroy(arena);
		return (0);
	}
	else
	{
		print_board(board);
		while (1)
		{
			play_one_turn(arena, board, game);
			print_board(board);
		}
		arena_destroy(arena);
		return (0);
	}
}