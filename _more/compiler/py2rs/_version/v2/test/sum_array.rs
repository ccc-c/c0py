use std::collections::HashMap;
fn sum_array(mut arr: Vec<i32>) -> i32 {
    let mut total = 0;
    for x in arr {
        total = total + x;
    }
    return total;
}

fn main() {
    let mut arr = vec![1, 2, 3, 4, 5];
    let result: i32 = sum_array(arr);
    println!("sum_array={}", result);
}