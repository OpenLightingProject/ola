<html>
<head>
 <title>LLAD - Info</title>
 <link rel="stylesheet" type="text/css" href="/simple.css"/>
<head>

<body>
 <span class="heading2">Plugin Info</span>

 <div align="center" style="margin-top: 40px">
  <table border="0" cellspacing="1" cellpadding="0" class="info" width="200px">
   <tr class="info_head">
    <th width="30px">ID</th>
    <th>Plugin Name</th>
   </tr>

    {{#NO_PLUGINS}}
     <tr><td colspan="2" align="center">No plugins loaded</td></tr>
    {{/NO_PLUGINS}}

    {{#PLUGIN}}
     <tr {{#ODD}}class="odd"{{/ODD}}>
      <td align="center">{{ID:html_escape}}</td>
      <td><a href="/plugin?id={{ID:url_query_escape}}">{{NAME:html_escape}}</a></td>
    </tr>
    {{/PLUGIN}}
  </table>
 </div>
</body>
</html>
