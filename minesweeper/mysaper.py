import random
import os


NEW_GAME = 1
LOAD_GAME = 2

SAVE_FILE_NAME = 'sapper.dat'

EASY = 1
MEDIUM = 2
HARD = 3
CUSTOM = 4

EASY_GAME = (5, 5, 5)
MEDIUM_GAME = (7, 7, 7)
HARD_GAME = (10, 10, 10)

CLOSED = 0
OPENED = 1
FLAG = 2
BOMB = 3
BLOW = 4

OPEN = 1
PUT = 2


class GameBoard():
    def __init__(self, rows, columns, bombs_count):
        assert(bombs_count <= rows*columns)
        self.__rows = rows
        self.__columns = columns
        self.__bombs_count = bombs_count
        self.__moves_count = 0
        self.__flags_count = 0
        self.flag_table = [[False for _ in range(columns)] for _ in range(rows)]
        self.table = [[CLOSED for _ in range(columns)] for _ in range(rows)]
        for _ in range(bombs_count):
            while True:
                x = random.randint(0, rows-1)
                y = random.randint(0, columns-1)
                if self.table[x][y] == CLOSED:
                    self.table[x][y] = BOMB
                    break

    def move(self, x, y, action):
        assert(action in [OPEN, PUT])
        assert(1 <= x <= self.rows)
        assert(1 <= y <= self.columns)
        x -= 1
        y -= 1
        if action == OPEN:
            self.open(x, y)
        elif action == PUT:
            self.put(x, y)

        if self.check_win():
            os.system('cls')
            self.game_win()

    def open(self, x, y):
        if self.table[x][y] == BOMB:
            self.table[x][y] = BLOW
            os.system('cls')
            self.print_table()
            self.game_over()

        if self.calc_bombs_count_near(x, y) == 0:
            self.open_near_cells(x, y)

        if self.table[x][y] == CLOSED and \
           not self.flag_table[x][y]:
            self.table[x][y] = OPENED

    def open_near_cells(self, x, y):
        dx = [-1, 1, 0, 0]
        dy = [0, 0, -1, 1]
        self.table[x][y] = OPENED
        for i in range(4):
            new_x = x + dx[i]
            new_y = y + dy[i]
            if self.border_check(new_x, new_y):
                if self.table[new_x][new_y] == CLOSED and \
                   not self.flag_table[new_x][new_y]:
                    if self.calc_bombs_count_near(new_x, new_y) == 0:
                        self.open_near_cells(new_x, new_y)

                    self.table[new_x][new_y] = OPENED

    def put(self, x, y):
        if self.table[x][y] != OPENED:
            if self.flag_table[x][y]:
                self.__flags_count -= 1
                self.flag_table[x][y] = False
            else:
                if self.__flags_count < self.bombs_count:
                    self.__flags_count += 1
                    self.flag_table[x][y] = True

    def check_win(self):
        count = 0
        for i in range(self.rows):
            for j in range(self.columns):
                if self.table[i][j] == OPENED or \
                   self.flag_table[i][j]:
                    count += 1

        return count == self.rows*self.columns

    def game_over(self):
        print('GAME OVER')
        exit()

    def game_win(self):
        print('Игра закончена.')
        print('Вы выиграли!')
        exit()

    def calc_bombs_count_near(self, x, y):
        dx = [-1, -1, -1, 0, 0, 1, 1, 1]
        dy = [0, 1, -1, -1, 1, 0, 1, -1]
        bombs_count = 0
        for i in range(8):
            new_x = x + dx[i]
            new_y = y + dy[i]
            if self.border_check(new_x, new_y):
                if self.table[new_x][new_y] in [BOMB, BLOW]:
                    bombs_count += 1

        return bombs_count

    def border_check(self, x, y):
        return 0 <= x < self.rows and 0 <= y < self.columns

    def print_table(self):
        convert = {
            CLOSED: '▓',
            BOMB: '▓',
            BLOW: '●',
            FLAG: '$',
        }
        for i in range(-2, self.rows+1):
            for j in range(-2, self.columns+1):
                if self.border_check(i, j):
                    if self.flag_table[i][j]:
                        print(convert[FLAG], end=' ')
                    elif self.table[i][j] == OPENED:
                        bombs_count = self.calc_bombs_count_near(i, j)
                        if bombs_count == 0:
                            print('░', end=' ')
                        else:
                            print(bombs_count, end=' ')
                    else:
                        print(convert[self.table[i][j]], end=' ')
                else:
                    if i == -2 and 0 <= j < self.columns:
                        print(str(j+1).ljust(2, ' '), end='')
                    elif j == -2 and 0 <= i < self.rows:
                        print(str(i+1).ljust(2, ' '), end='')
                    elif i == -1 and j == -1:
                        print('╔', end='')
                    elif i == -1 and j == self.columns:
                        print('╗', end='')
                    elif j == -1 and i == self.rows:
                        print('╚', end='')
                    elif j == self.columns and i == self.rows:
                        print('╝', end='')
                    elif i == -1 or i == self.rows:
                        print('══', end='')
                    elif j == -1 or j == self.columns:
                        if i == -2 and j == -1:
                            print(end='  ')
                        print('║', end='')
            print()

    @property
    def rows(self):
        return self.__rows

    @property
    def columns(self):
        return self.__columns

    @property
    def bombs_count(self):
        return self.__bombs_count

    @property
    def moves_count(self):
        return self.__moves_count


def save_game(game: GameBoard):
    with open(SAVE_FILE_NAME, 'w') as file:
        file.write(f'{game.rows} {game.columns} {game.bombs_count}\n')
        for i in range(game.rows):
            for j in range(game.columns):
                file.write(str(game.table[i][j]) + ' ')
            file.write('\n')


def game_start(game: GameBoard):
    print(game.table)
    while True:
        os.system('cls')
        game.print_table()
        print('1. Открыть ячейку')
        print('2. Поставить флаг')
        print('Введите координаты и действие:')
        print('Чтобы сохранить игры введите 28')
        text = input('Введите: (x, y, action) ')
        if text.strip() == '28':
            save_game(game)
            print('Успешно сохранено!')
        else:
            try:
                x, y, action = map(int, text.split())
                game.move(x, y, action)
            except Exception:
                print('Входные данные некорректны!')


def new_game():
    print('Выберите:')
    print('    1. Легкий')
    print('    2. Средний')
    print('    3. Сложный')
    print('    4. Вручную')
    while True:
        action = int(input('Введите: '))
        if action in [EASY, MEDIUM, HARD, CUSTOM]:
            break

    if action == EASY:
        game = GameBoard(*EASY_GAME)
    elif action == MEDIUM:
        game = GameBoard(*MEDIUM_GAME)
    elif action == HARD:
        game = GameBoard(*HARD_GAME)
    elif action == CUSTOM:
        print('Введите в строку через пробел: Размерности(количество строк и столбцов) и количество бомб.')
        while True:
            try:
                rows, columns, bombs_count = map(int, input('Введите: ').split())
                game = GameBoard(rows, columns, bombs_count)
            except Exception:
                pass
            else:
                break

    game_start(game)


def load_game():
    try:
        with open(SAVE_FILE_NAME, 'r') as file:
            rows, columns, bombs_count = map(int, file.readline().split())
            game = GameBoard(rows, columns, bombs_count)
            table = []
            for _ in range(rows):
                table.append(list(map(int, file.readline().split())))

            game.table = table
            game_start(game)
    except FileNotFoundError:
        print('Файл не найден!')
    except Exception as e:
        print(repr(e))
        print('Ошибка при загрузки игры. Файл поврежден.')


def main():
    print('Добро пожаловать в игру сапёр')
    print('Выберите:')
    print('    1. Новая игра')
    print('    2. Загрузить последнюю сохранённую игру')
    while True:
        action = int(input('Введите: '))
        if action in [NEW_GAME, LOAD_GAME]:
            break

    if action == NEW_GAME:
        new_game()
    elif action == LOAD_GAME:
        load_game()


if __name__ == "__main__":
    main()
