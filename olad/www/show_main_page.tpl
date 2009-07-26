<html>
 <head>
  <title>OLAD - Info</title>
  <link rel="stylesheet" type="text/css" href="/simple.css"/>
 <head>

 <body>

 <span class="heading2">System Info</span>


 <div align="center" style="margin-top: 40px">
   <table border="0" cellspacing="1" cellpadding="0" class="sys_info">
    <tr>
     <td>IP Address:</td>
     <td><TMPL_VAR NAME="ip"></td>
    </tr>
    <tr>
     <td>Uptime:</td>
     <td><TMPL_VAR NAME="uptime"></td>
    </tr>
    <tr>
     <td>Load Average:</td>
     <td><TMPL_VAR NAME="load"></td>
    </tr>
  </table>

  <br/>

  {{#QUIT_ENABLED}}
  <a href="/quit">Stop Olad</a>
  {{/QUIT_ENABLED}}

  </div>
 </body>
</html>
