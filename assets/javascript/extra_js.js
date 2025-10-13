document$.subscribe(function () {
  var tables = document.querySelectorAll("article table:not([class])");
  tables.forEach(function (table) {
    // The only way to have only two rows is to have either:
    // - a header with one row (no need for sorting)
    // - no header with two rows (again, no need for soring, since header is hidden)
    if (table.querySelectorAll('tr').length === 2) return;

    new Tablesort(table);
  });
});

document$.subscribe(({ body }) => {
  renderMathInElement(body, {
    delimiters: [
      { left: "$$", right: "$$", display: true },
      { left: "$", right: "$", display: false },
      { left: "\\(", right: "\\)", display: false },
      { left: "\\[", right: "\\]", display: true },
    ],
  });
});
