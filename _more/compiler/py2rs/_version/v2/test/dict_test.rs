use std::collections::HashMap;

fn main() {
    let mut d: HashMap<String, i32> = HashMap::from([("a".to_string(), 1), ("b".to_string(), 2)]);
    println!("dict:{:?}", d);
}