/**
 * @file
 * Code for the supercookie module.
 */

(function ($) {

  'use strict';

  Drupal.behaviors.supercookie = {
    entitiesTracked: false,
    geoLocation: false,
    attach : function(context, settings) {

      // Honor user's DNT header.
      if (settings.supercookie.dnt === true) {
        return;
      }

      // Add request header to all AJAX requests.
      $.ajaxSetup({
        global: true,
        beforeSend: function(xhr, options) {
          if (options.url.indexOf(settings.supercookie.json) < 0) {

            if (!settings.supercookie.hash) {
              // This condition occurs when the original writeCookie() call
              // below fails due to high security browser settings; we'll assume
              // the user still has JavaScript enabled, though and force our
              // headers upon them.
              Drupal.behaviors.supercookie.failOver(context, settings, xhr, options);
            }
            else {
              xhr.setRequestHeader(settings.supercookie.name_header, settings.supercookie.hash);
              if (options.headers) {
                options.headers[settings.supercookie.name_header] = settings.supercookie.hash;
              }
            }

          }
        }
      });

      this.writeCookie(context, settings);

    },
    failOver: function(context, settings, xhr, xhrOptions) {

      // Kill current AJAX request.
      xhr.abort();

      // Force a new supercookie request first.
       $.when(this.writeCookie(context, settings))
        .then(function(data) {
          // Force the supercookie request header on to the original request.
          xhrOptions.headers[settings.supercookie.name_header] = data.hash;
          // Now send the original (manipulated) request.
          $.ajax(xhrOptions);
        });

    },
    writeCookie : function(context, settings) {

      if ($.isEmptyObject(navigator)) {
        return false;
      }

      // Loop navigator object and fingerprint this user.
      var data = {};
      for (var member in navigator) {
        // 2016-04-01, Tory:
        // Firefox - a private browsing window does not have this property on
        // navigator object, while a normal window does. Bypass it so our
        // fingerprint is not different between the two window modes.
        // @see https://bugzilla.mozilla.org/show_bug.cgi?id=1112136
        if (member === 'serviceWorker') {
          continue;
        }

        switch (typeof navigator[member]) {
          case 'object':
          case 'string':
          case 'boolean':
            data[member] = navigator[member];
            break;
        }
      }
      // Do deep recursion on data collected.
      data = JSON.prune(data);
      // Set hash of data string.
      var hash = CryptoJS.MD5(data);
      // Get local date/time. TODO: NOTE SAFARI AND IE'S MISHANDLING OF ECMA DATE OBJECT FORMAT!!!!
      var date = new Date($.now());
      date = date.toLocaleString();
      date = encodeURIComponent(date);

      var url = settings.supercookie.json + '?client=' + hash + '&date=' + date;
      if (!this.entitiesTracked) {
        url += '&ref=' + document.location.pathname.substr(1);
      }
      // TODO: Thread in conditional navigator.geolocation url params, expand
      // supercookie table to include dedicated geo blob.

      // Get server response.
      return $.ajax({
        method : 'GET',
        url : url,
        beforeSend: function(xhr, options) {
          xhr.setRequestHeader(settings.supercookie.name_header, settings.supercookie.hash);
          xhr.setRequestHeader('Accept', 'application/json');

          // Ensures that the "ref" query param is only sent to the server once.
          Drupal.behaviors.supercookie.entitiesTracked = true;
        },
        complete : function(xhr) {
          if (xhr.status === 200) {
            // Set client-side cookie + localStorage.
            var response = (xhr.responseJSON ? xhr.responseJSON : JSON.parse(xhr.responseText));
            var expires = new Date(response.expires * 1000);

            document.cookie = settings.supercookie.name_server + '=""; expires=-1';
            document.cookie = settings.supercookie.name_server + '=' + response.hash + '; expires=' + expires.toGMTString() + '; path=/';
            try {
              window.localStorage.setItem(settings.supercookie.name_server, response.hash);
            }
            catch(ex) { }

            Drupal.settings.supercookie.scid = settings.supercookie.scid = response.scid;
            Drupal.settings.supercookie.hash = settings.supercookie.hash = response.hash;

            return response.hash;
          }
        }
      });

    }
  };

})(jQuery);
