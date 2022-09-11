import random
import sys
sys.setrecursionlimit(1_500_000)
WIDTH = 40
HEIGHT = 40
VISITED = 1

def is_empty(board, x, y):
    return board[y][x] != VISITED


def neighbours4_(pos):
    x = pos[0]
    y = pos[1]
    return [(x - 1, y), (x, y - 1), (x, y + 1), (x + 1, y)]


def neighbours_corners_(pos):
    x = pos[0]
    y = pos[1]
    return [(x - 1, y - 1), (x - 1, y + 1), (x + 1, y - 1), (x + 1, y + 1)]


def neighbours4(pos):
    return list(
        filter(lambda pos: pos[0] >= 0 and pos[0] < WIDTH and
                           pos[1] >= 0 and pos[1] < HEIGHT, neighbours4_(pos)))


def neighbours_corners(pos):
    return list(
        filter(lambda pos: pos[0] >= 0 and pos[0] < WIDTH and
                           pos[1] >= 0 and pos[1] < HEIGHT,
               neighbours_corners_(pos)))


def next_possible_cells(board, last_pos):
    return list(
        filter(
            lambda pos:
            is_empty(board, pos[0], pos[1]) and
            all(map(lambda pos: (pos[0] == last_pos[0] and pos[1] == last_pos[1]) or
                                is_empty(board, pos[0], pos[1]), neighbours4(pos))) and
            all(map(lambda pos: is_empty(board, pos[0], pos[1]) or pos in neighbours4(last_pos),
                    neighbours_corners(pos))), neighbours4(last_pos)))

def next_pos(board, last_pos):
    cells = next_possible_cells(board, last_pos)
    return random.choice(cells) if cells else None

def make_path(board, stack_path):
    if stack_path:
        new_pos = next_pos(board, stack_path[0])
        if new_pos:
            board[new_pos[1]][new_pos[0]] = VISITED
            stack_path.insert(0, new_pos)
            return make_path(board, stack_path)
        else:
            return make_path(board, stack_path[1:])

board = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]
start_pos_x = random.randrange(WIDTH)
start_pos_y = random.randrange(HEIGHT)
board[start_pos_y][start_pos_x] = VISITED
make_path(board, [(start_pos_x, start_pos_y)])
print(board)