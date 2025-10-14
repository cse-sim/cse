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

// The script probably would not affect the multi-page site, since the paragraphs containing
// @nested-dl should have already been removed by beautiful soup in the on_page_content
// hook, but adding "section.print-page" to the selector just adds a layer of confidence
// that the JS will only operate on elements that are part of the single-page site.
document.querySelectorAll("section.print-page p:has(+ :is(h1, h2, h3, h4, h5, h6))").forEach(element => {
  if (element.innerHTML !== '@nested-dl') return

  let currentElement = element.nextElementSibling
  const heading_level = currentElement?.tagName.startsWith("H") ? currentElement.tagName : null

  if (!heading_level) return

  while (currentElement) {
    if (currentElement.tagName === 'DL') {
      currentElement.classList.add("nested-dl")
    }

    currentElement = currentElement.nextElementSibling
    if (currentElement?.tagName === heading_level) {
      break
    }
  }

  element.remove()
})


