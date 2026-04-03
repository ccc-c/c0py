use std::collections::HashMap;
fn factorial_while(mut n: i32) -> i32 {
    let mut result = 1;
    while n > 1 {
        result = (result * n);
        n = (n - 1);
    }
    return result;
}

fn main() {
    let result = factorial_while(5);
    println!("factorial_while(5)={}", result);
}