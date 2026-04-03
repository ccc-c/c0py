struct Counter {
    count: i32,
}

impl Counter {
    fn new() -> Counter {
        Counter { count: 0 }
    }

    fn increment(&mut self) -> i32 {
        self.count += 1;
        self.count
    }

    fn get_count(&self) -> i32 {
        self.count
    }
}

fn main() {
    let mut c = Counter::new();
    c.increment();
    c.increment();
    println!("count: {}", c.get_count());
}