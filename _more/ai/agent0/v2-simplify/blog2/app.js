const express = require('express');
const Database = require('better-sqlite3');
const path = require('path');

const app = express();
const db = new Database('blog.db');

app.set('view engine', 'ejs');
app.use(express.urlencoded({ extended: true }));

db.exec(`
  CREATE TABLE IF NOT EXISTS posts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT,
    content TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
  )
`);

app.get('/', (req, res) => {
  const posts = db.prepare('SELECT * FROM posts ORDER BY created_at DESC').all();
  res.render('index', { posts });
});

app.get('/new', (req, res) => {
  res.render('new');
});

app.post('/new', (req, res) => {
  const { title, content } = req.body;
  db.prepare('INSERT INTO posts (title, content) VALUES (?, ?)').run(title, content);
  res.redirect('/');
});

app.listen(3000, () => {
  console.log('Blog running on http://localhost:3000');
});
