use std::collections::HashMap;
fn circle_area(r: f64) -> f64 {
    return ((std::f64::consts::PI * r) * r);
}

fn main() {
    let result: f64 = circle_area(2.0);
    println!("circle_area(2.0)={}", result);
}