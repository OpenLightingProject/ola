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
<title>Plugin config files for the Open Lighting Architecture</title>
</head>
<body>
<h1>Plugin config files for the <a href="https://openlighting.org/ola/">Open Lighting Architecture</a></h1>
<ul>
EOF

config_prefix=$(grep ::OLA_CONFIG_PREFIX ../olad/plugin_api/Preferences.cpp | sed -e 's/const char FileBackedPreferences::OLA_CONFIG_PREFIX\[\] = "//' -e 's/";$//');
echo "Config prefix: $config_prefix";
config_suffix=$(grep ::OLA_CONFIG_SUFFIX ../olad/plugin_api/Preferences.cpp | sed -e 's/const char FileBackedPreferences::OLA_CONFIG_SUFFIX\[\] = "//' -e 's/";$//');
echo "Config suffix: $config_suffix";

for readme_file in */README.md; do
  echo "Generating from $readme_file";
  plugin_dir=$(dirname $readme_file);
  echo "Plugin dir: $plugin_dir";
  plugin_name=$(grep ::PLUGIN_NAME $plugin_dir/*Plugin.cpp | sed -e 's/const char .*Plugin::PLUGIN_NAME\[\] = "//' -e 's/";$//');
  echo "Plugin name: $plugin_name";
  plugin_prefix=$(grep ::PLUGIN_PREFIX $plugin_dir/*Plugin.cpp | sed -e 's/const char .*Plugin::PLUGIN_PREFIX\[\] = "//' -e 's/";$//');
  echo "Plugin prefix: $plugin_prefix";
  config_file=$config_prefix$plugin_prefix$config_suffix;
  output_file=$output_dir/$config_file.html;
  page_title="$plugin_name Plugin ($config_file)";
  echo "Config file: $config_file";
  pandoc -f markdown_github -t html -s $plugin_dir/README.md -B pandoc-html-index.html -A pandoc-html-index.html -o $output_file -T "$page_title"
  sed -i -e 's/ – <\/title>/<\/title>/' $output_file
  chmod a+r $output_file;
  echo "<li><a href='./$config_file.html'>$page_title</a></li>" >> $index_file
done

cat << 'EOF' >> $index_file
</ul>
</body>
</html>
EOF

chmod a+r $index_file;
