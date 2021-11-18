---
title: Writeups
layout: page
permalink: /writeups/
---

{% assign sorted = site.writeups | reverse %}
{% for t in sorted %}
<h3 class="post-item-title">
    <a href="{{ t.url }}">{{ t.title | escape }}</a>
    
</h3> 
{% endfor %}