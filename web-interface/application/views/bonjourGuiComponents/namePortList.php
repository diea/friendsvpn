<ul class="nav nav-pills nav-stacked">
  <?php
  foreach($namePorts as $namePort) {
      echo "<li class=\"liport\">";
      ?>
      <a href="#" class="portli" 
                  data-name="<?php echo $namePort["name"]; ?>"
                  data-port="<?php echo $namePort["port"]; ?>"
                  data-transprot="<?php echo $namePort["Service_Trans_prot"]; ?>">
        <?php echo $namePort["name"] . " on port " . $namePort["port"] . " using " . $namePort["Service_Trans_prot"]; ?>
      </a>
      <?php
      echo "</li>";
  }
  ?>
</ul>

<script>
    /* ajax call for the services offered by a given host */
    $(".portli").click(function(event) {
        event.preventDefault();
        //alert($(this).html());
        $(".liport").removeClass("active");
        $(this).parent().addClass("active");
        var hostname = $(".active.lihost").children().first().html();
        var service = $(".active.liservice").children().first().html();
        var name = $(this).data("name");
        var port = $(this).data("port");
        var transProt = $(this).data("transprot");
        $.ajax({
            type: "post",
            url:"bonjourgui/details/",
            data: {
                "service": service,
                "hostname": hostname,
                "name": name,
                "port": port,
                "transProt": transProt
            }
        }).done(function(data) {
            $(".permissionsField").html(data);
        });
    });
</script>
