/**
 * @file
 *
 * JavaScript functionalities for the Semantic Connector frontend.
 */

(function ($) {
  Drupal.behaviors.semanticConnectorConceptDestinations = {
    attach: function (context) {
      // check if the individual PP servers are available.
      $("div.semantic-connector-led").each(function() {
        setLed($(this));
      });

      // Add all the actions required for the concept destinations menu.
      $(".semantic-connector-concept").each(function() {
        if ($(this).find('ul.semantic-connector-concept-destination-links').length > 0) {
          $(this).find('a.semantic-connector-concept-link').click(function () {
            $(this).siblings('ul.semantic-connector-concept-destination-links').show();
            return false;
          });
        }
      });
      $(".semantic-connector-concept-destination-links").mouseover(function() {
        $(this).show();
      });
      $(".semantic-connector-concept").mouseout(function() {
        $(this).find('.semantic-connector-concept-destination-links').hide();
      });
    }
  };

  var setLed = function(item) {
    var url = Drupal.settings.basePath + "admin/config/semantic-drupal/semantic-connector/connections/" + item.data("server-type") + "/" + item.data("server-id") + "/available";
    $.get(url, function (data) {
      var led = "led-red";
      var title = Drupal.t("Service NOT available");
      if (data == 1) {
        led = "led-green";
        title = Drupal.t("Service available");
      }
      item.addClass(led);
      item.attr("title", title);
    });
  };
})(jQuery);


