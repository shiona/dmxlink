#ifndef RADIO_CONSTANTS_H
#define RADIO_CONSTANTS_H

// Nää on semmosia joita ois kiva tietää sekä tuolla lähettäjällä
// et vastaanottajalla
const uint8_t CHIP_ENABLE_PIN = A0;
const uint8_t CHIP_SELECT_PIN = 10;

// Nää on valittu ihan ns. läpällä
// Silti järkevähköä että dataputki 1 lukemiseen
// Tää on yhteinen osa kaikissa radionumeroissa
const uint64_t RADIO_DATA_PIPE_MASK = 0xF0F0F0F000LL;
const uint8_t RADIO_DATA_PIPE_NUM = 1;
// Periaatteessa meillä ei oo telemetriaa
const uint8_t RADIO_TELEMETRY_PIPE_NUM = 2;
const uint64_t RADIO_TELEMETRY_PIPE_MASK = 0xF1F1F1F100LL;

// Muotoa (RETRY_DELAY + 1) * 250 mikrosekuntia
// Tämä arvo siis (15 + 1) * .250 = 4 millisekuntia
const uint8_t RADIO_RETRY_DELAY = 15;
const uint8_t RADIO_MAX_RETRIES = 15;

// Kanava, jota käytetään
// Väliltä 0-127, oletetaan et 0 toimii? Toivottavasti
// oletus on 4C en tiä miks
const uint8_t RADIO_RF_CHANNEL = 0x4C ;

#endif
