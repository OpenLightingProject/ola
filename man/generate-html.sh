#!/bin/sh

if [ $# != 1 ]; then
  echo "Usage: $0 <output_dir>"
  exit 1;
fi

output_dir=$1
if [ ! -d $output_dir ]; then
  echo $output_dir is not a directory
  exit 1;
fi

index_file=$output_dir/index.html;

cat << 'EOF' > $index_file
<html>
<body>
<h1>Man pages for the Open Lighting Project</h3>
<ul>
EOF

for man_file in *.1; do
  man2html -r $man_file | sed 1d > "$output_dir/man1/$man_file.html";
  echo "<li><a href='./man1/$man_file.html'>$man_file</a></li>" >> $index_file
done

cat << 'EOF' >> $index_file
</ul>
</body>
</html>
EOF
