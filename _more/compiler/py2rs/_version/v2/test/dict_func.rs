use std::collections::HashMap;
fn get_value(d: HashMap<String, i32>, key: String) -> i32 {
    return d.get(&key).copied().unwrap_or(0);
}

fn main() {
    let mut d: HashMap<String, i32> = HashMap::from([("a".to_string(), 10), ("b".to_string(), 20), ("c".to_string(), 30)]);
    let result: i32 = get_value(d, "b".to_string());
    println!("get_value(b)={}", result);
}