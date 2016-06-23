/**
 * @file
 *
 * JavaScript functionalities for the Smart Glossary backend.
 */

(function ($) {
  Drupal.behaviors.smart_glossary = {
    attach: function (context) {

      var basePath = $("#edit-base-path");
      if (basePath.length) {
        basePath.before($("<span>" + Drupal.settings.smart_glossary.baseUrl + Drupal.settings.basePath + "</span>"))
      }

      // Select all content if the textarea field is focused.
      var rdf = $("#edit-advanced-settings-rdf-text");
      if (rdf.length) {
        rdf.focus(function () {
          var $this = $(this);
          $this.select();

          // Work around Chrome's little problem
          $this.mouseup(function () {
            // Prevent further mouseup intervention
            $this.unbind("mouseup");
            return false;
          });
        });
      }

      // Make the project tables sortable if tablesorter is available.
      if ($.isFunction($.fn.tablesorter)) {
        $("table#smart-glossary-configurations-table").tablesorter({
          widgets: ["zebra"],
          widgetOptions: {
            zebra: ["odd", "even"]
          },
          sortList: [[0, 0]],
          headers: {
            3: {sorter: false},
            4: {sorter: false}
          }
        });
      }
    }
  };
})(jQuery);
