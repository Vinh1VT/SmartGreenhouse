function decodeUplink(input) {
  var bytes = input.bytes;
  var decoded = {};
  var i = 0;
  var anomaly = false;

  while (i < bytes.length) {
    var header = String.fromCharCode(bytes[i]);
    i++;

    switch (header) {
      // ------------------------------------------------
      // [T] BLOC TEMPÉRATURES (2 octets par valeur)
      // Correspond à l'ordre dans sensor_Data.ino
      // ------------------------------------------------
      case 'T':
        var t_names = [
          'temperature_ambiant', 
          'temperature_puit_sortie', 
          'temperature_puit_entree', 
          'temperature_terre_1', 
          'temperature_terre_2'
        ];
        var t_len = t_names.length; 
        
        if (i + (t_len * 2) > bytes.length) { anomaly = true; i = bytes.length; break; }
        
        for (var j = 0; j < t_len; j++) {
          var t_val = (bytes[i + j*2] << 8) | bytes[i + j*2 + 1];
          if (t_val === 0x7FFF) { anomaly = true; } 
          else if (t_val !== 0x7FFE) { 
            if (t_val & 0x8000) t_val -= 0x10000; // Gestion des nombres négatifs
            decoded[t_names[j]] = t_val / 10.0;
          }
        }
        i += (t_len * 2);
        break;

      // ------------------------------------------------
      // [H] BLOC HUMIDITÉS (1 octet par valeur)
      // ------------------------------------------------
      case 'H':
        var h_names = [ 
          'humidite_ambiant', 
          'humidite_oya_1', 'humidite_oya_2', 
          'humidite_oya_3', 'humidite_oya_4', 
          'humidite_bille_1', 'humidite_bille_2'
        ];
        var h_len = h_names.length;
        
        if (i + h_len > bytes.length) { anomaly = true; i = bytes.length; break; }
        
        for (var k = 0; k < h_len; k++) {
          var h_val = bytes[i + k];
          if (h_val === 0xFF) { anomaly = true; } 
          else if (h_val !== 0xFE) { decoded[h_names[k]] = h_val; } 
        }
        i += h_len;
        break;

      // ------------------------------------------------
      // [L] BLOC LUMINANCES (2 octets)
      // ------------------------------------------------
      case 'L':
        if (i + 2 > bytes.length) { anomaly = true; i = bytes.length; break; }
        var l_val = (bytes[i] << 8) | bytes[i + 1];
        if (l_val === 0xFFFF) { anomaly = true; }
        else if (l_val !== 0xFFFE) { decoded.luminance_ambiant = l_val; }
        i += 2;
        break;

      // ------------------------------------------------
      // [G] BLOC GAZ (CO2 - 2 octets)
      // ------------------------------------------------
      case 'G':
        if (i + 2 > bytes.length) { anomaly = true; i = bytes.length; break; }
        var co2 = (bytes[i] << 8) | bytes[i + 1];
        if (co2 === 0xFFFF) { anomaly = true; }
        else if (co2 !== 0xFFFE) { decoded.co2 = co2; }
        i += 2;
        break;

      // ------------------------------------------------
      // [B] BLOC BATTERIE (2 octets)
      // ------------------------------------------------
      case 'B':
        if (i + 2 > bytes.length) { anomaly = true; i = bytes.length; break; }
        var batt = (bytes[i] << 8) | bytes[i + 1];
        if (batt !== 0xFFFE) { 
          decoded.batterie_pourcentage = batt / 10.0; // Stocké en % * 10 dans l'ESP32
        }
        i += 2;
        break;

      // ------------------------------------------------
      // [I] BLOC IA (4 valeurs de 2 octets chacune)
      // ------------------------------------------------
      case 'I':
        var i_names = ['ia_val_1', 'ia_val_2', 'ia_val_3', 'ia_anomalie'];
        if (i + 8 > bytes.length) { anomaly = true; i = bytes.length; break; }
        
        for (var n = 0; n < 4; n++) {
          var i_val = (bytes[i + n*2] << 8) | bytes[i + n*2 + 1];
          if (i_val !== 0xFFFE) {
            decoded[i_names[n]] = i_val / 100.0; // Les données IA sont multipliées par 100 à l'envoi
          }
        }
        i += 8;
        break;

      default:
        anomaly = true;
        i = bytes.length;
        break;
    }
  }

  decoded.sensor_anomaly = anomaly;

  return {
    data: decoded,
    warnings: [],
    errors: []
  };
}