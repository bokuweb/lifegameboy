use alloc::vec::Vec;

type Field<T> = Vec<Vec<T>>;

struct Rand {
    x: usize,
}

impl Rand {
    pub fn new(x: usize) -> Self {
        Self { x }
    }
    fn rand(&mut self) -> usize {
        self.x = self.x * 1103515245 + 12345;
        return self.x & 2147483647;
    }
}

pub struct Game {
    row_size: usize,
    col_size: usize,
    field: Field<bool>,
}

impl Game {
    pub fn new(row_size: usize, col_size: usize) -> Game {
        let mut rand = Rand::new(1);
        let buf: Vec<bool> = (0..col_size * row_size).map(|_i| rand.rand() % 10 < 6).collect();
        let field = Game::create_field(&buf, row_size, col_size);
        Game { field, row_size, col_size }
    }

    pub fn next(&mut self) -> Vec<bool> {
        let next: Vec<bool> = self.field.iter().enumerate().map(|(y, r)| self.next_row(r, y)).flat_map(|x| x).collect();
        self.field = Game::create_field(&next, self.row_size, self.col_size);
        next
    }

    fn next_row(&self, row: &Vec<bool>, y: usize) -> Vec<bool> {
        row.iter().enumerate().map(|(x, _)| self.next_cell(y as i32, x as i32)).collect()
    }

    fn next_cell(&self, y: i32, x: i32) -> bool {
        let alive_num = [
            (y - 1, x - 1),
            (y, x - 1),
            (y + 1, x - 1),
            (y - 1, x),
            (y + 1, x),
            (y - 1, x + 1),
            (y, x + 1),
            (y + 1, x + 1),
        ]
        .iter()
        .map(|&(y, x)| self.get_cell_state(y, x))
        .filter(|cell| *cell)
        .collect::<Vec<_>>()
        .len();
        match alive_num {
            3 => true,
            2 if self.is_alive(y as usize, x as usize) => true,
            _ => false,
        }
    }

    fn is_alive(&self, y: usize, x: usize) -> bool {
        self.field[y][x]
    }

    fn create_field(buf: &[bool], row_size: usize, col_size: usize) -> Field<bool> {
        (0..row_size)
            .into_iter()
            .map(|i| {
                let start = i * col_size;
                let end = start + col_size;
                buf[start..end].to_vec()
            })
            .collect()
    }

    fn get_cell_state(&self, row: i32, column: i32) -> bool {
        match self.field.get(row as usize) {
            Some(r) => match r.get(column as usize) {
                Some(c) => *c,
                None => false,
            },
            None => false,
        }
    }
}
