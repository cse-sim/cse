---
title: "California Simulation Engine"
toc: false
---

# Overview {-}

CSE is a general purpose building simulation model developed primarily to perform the required calculations for the California Building Energy Code Compliance for Residential buildings ([CBECC-Res](http://www.bwilcox.com/BEES/BEES.html)) software.

## CSE User Manual {-}

- [HTML Format](cse-user-manual/index.html) (multiple pages)
- [HTML Format](cse-user-manual.html) (single page)

## CSE Source Code {-}

The CSE source code is hosted on [GitHub](https://github.com/cse-sim/cse).

```javascript
const myVar = () => {};
```

<% if test_erb %>
ERB is Working!
<% end %>

{% for year in ['2018', '2017'] %}

  <h3>{{ year }}</h3>
  {% for page in pages|sort(attribute='url', reverse=True) %}
    {{ page.title }}
    <br />
  {% endfor %}
{% endfor %}
