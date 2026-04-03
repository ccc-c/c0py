fn main() {
    let numbers = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    // List comprehension with condition
    let even_squares: Vec<i32> = numbers.iter()
        .filter(|x| *x % 2 == 0)
        .map(|x| x * x)
        .collect();
    println!("even_squares:{:?}", even_squares);

    // Filter and map
    let filtered: Vec<i32> = numbers.iter()
        .filter(|x| *x > 5)
        .cloned()
        .collect();
    println!("filtered:{:?}", filtered);

    // Nested list
    let matrix: Vec<Vec<i32>> = (0..3)
        .map(|i| {
            (0..3)
                .map(|j| i * 3 + j)
                .collect()
        })
        .collect();
    println!("matrix:{:?}", matrix);
}