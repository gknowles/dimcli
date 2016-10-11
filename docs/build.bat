:: gem install kramdown
:: gem install rouge
kramdown --input GFM README.md >index.html ^
  --template index.erb ^
  --syntax-highlighter rouge ^
  --syntax-highlighter-opts "{code-class: 'codehilite', css-class: 'codehilite'}"
