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

echo "Output dir: $output_dir";
echo "Index file: $index_file";

cat << 'EOF' > $index_file
<html>
<head>
<title>Man pages for the Open Lighting Architecture</title>
</head>
<body>
<h1>Man pages for the <a href="https://openlighting.org/ola/">Open Lighting Architecture</a></h1>
<ul>
EOF

if [ ! -d $output_dir/man1/ ]; then
  echo $output_dir/man1/ doesn\'t exist, please create it
  exit 1;
fi
for man_file in *.1; do
  echo "Generating $man_file";
  output_file=$output_dir/man1/$man_file.html;
  man2html -r $man_file -M ../index.html | sed 1,2d > $output_file;
  chmod a+r $output_file;
  echo "<li><a href='./man1/$man_file.html'>$man_file</a></li>" >> $index_file
done

cat << 'EOF' >> $index_file
</ul>
</body>
</html>
EOF

chmod a+r $index_file;
