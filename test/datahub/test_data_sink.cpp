// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <future>
#include <chrono>
#include <deque>
#include <atomic>
#include <iostream>

#include "scheduler.hpp"
#include "data/bybit/entities/public_trade.hpp"
#include "data/bybit/entities/response.hpp"
#include "connect/connection_context.hpp"
#include "datahub/data_sink.hpp"
#include "datahub/sync_policy.hpp"


using namespace scratcher;
using namespace scratcher::datahub;

static std::string websock_samples[] = {
  R"({"success":true,"ret_msg":"subscribe","conn_id":"11937fe0-c938-4a8b-b313-81fe27cfa8f6","op":"subscribe"})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520812165,"type":"snapshot","data":[{"i":"2240000000970262966","T":1761520812163,"p":"114513.1","v":"0.00862","S":"Sell","seq":111156121170,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970262967","T":1761520812163,"p":"114513.1","v":"0.05548","S":"Sell","seq":111156121170,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520812170,"type":"snapshot","data":[{"i":"2240000000970262969","T":1761520812168,"p":"114513.1","v":"0.0001","S":"Sell","seq":111156121214,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520815667,"type":"snapshot","data":[{"i":"2240000000970263000","T":1761520815666,"p":"114513.2","v":"0.004332","S":"Buy","seq":111156126558,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263001","T":1761520815666,"p":"114513.2","v":"0.017378","S":"Buy","seq":111156126558,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520815667,"type":"snapshot","data":[{"i":"2240000000970263002","T":1761520815666,"p":"114513.2","v":"0.0018","S":"Buy","seq":111156126561,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520815668,"type":"snapshot","data":[{"i":"2240000000970263003","T":1761520815666,"p":"114513.2","v":"0.013077","S":"Buy","seq":111156126562,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263004","T":1761520815666,"p":"114513.2","v":"0.002593","S":"Buy","seq":111156126562,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520815668,"type":"snapshot","data":[{"i":"2240000000970263005","T":1761520815667,"p":"114513.2","v":"0.010829","S":"Buy","seq":111156126572,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520815895,"type":"snapshot","data":[{"i":"2240000000970263011","T":1761520815894,"p":"114518.7","v":"0.004366","S":"Buy","seq":111156128085,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263012","T":1761520815894,"p":"114518.7","v":"0.032255","S":"Buy","seq":111156128085,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263013","T":1761520815894,"p":"114518.7","v":"0.013379","S":"Buy","seq":111156128085,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520816237,"type":"snapshot","data":[{"i":"2240000000970263021","T":1761520816236,"p":"114524.3","v":"0.0498","S":"Sell","seq":111156129747,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520821233,"type":"snapshot","data":[{"i":"2240000000970263041","T":1761520821231,"p":"114524.3","v":"0.2002","S":"Sell","seq":111156136564,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263042","T":1761520821231,"p":"114524.3","v":"0.0498","S":"Sell","seq":111156136564,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520824431,"type":"snapshot","data":[{"i":"2240000000970263072","T":1761520824429,"p":"114512.8","v":"0.004366","S":"Buy","seq":111156141733,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263073","T":1761520824429,"p":"114512.8","v":"0.032234","S":"Buy","seq":111156141733,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520824481,"type":"snapshot","data":[{"i":"2240000000970263074","T":1761520824480,"p":"114512.8","v":"0.000021","S":"Buy","seq":111156141931,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520826613,"type":"snapshot","data":[{"i":"2240000000970263103","T":1761520826611,"p":"114518.6","v":"0.03","S":"Sell","seq":111156145474,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520829153,"type":"snapshot","data":[{"i":"2240000000970263111","T":1761520829152,"p":"114520.5","v":"0.000031","S":"Sell","seq":111156149309,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520832013,"type":"snapshot","data":[{"i":"2240000000970263135","T":1761520832011,"p":"114520.6","v":"0.004366","S":"Buy","seq":111156152059,"s":"BTCUSDC","BT":false,"RPI":false},{"i":"2240000000970263136","T":1761520832011,"p":"114520.6","v":"0.000834","S":"Buy","seq":111156152059,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520834322,"type":"snapshot","data":[{"i":"2240000000970263161","T":1761520834321,"p":"114524.4","v":"0.000027","S":"Buy","seq":111156155379,"s":"BTCUSDC","BT":false,"RPI":false}]})",
  R"({"topic":"publicTrade.BTCUSDC","ts":1761520834910,"type":"snapshot","data":[{"i":"2240000000970263163","T":1761520834909,"p":"114524.4","v":"0.02335","S":"Buy","seq":111156155862,"s":"BTCUSDC","BT":false,"RPI":false}]})",
};

static std::string http_samples[] = {
  R"({"retCode":0,"retMsg":"OK","result":{"category":"spot","list":[{"execId":"2240000000970262766","symbol":"BTCUSDC","price":"114505.5","size":"0.000036","side":"Buy","time":"1761520773879","isBlockTrade":false,"isRPITrade":false,"seq":"111156078262"},{"execId":"2240000000970262748","symbol":"BTCUSDC","price":"114497.5","size":"0.0641","side":"Buy","time":"1761520772305","isBlockTrade":false,"isRPITrade":false,"seq":"111156075736"},{"execId":"2240000000970262747","symbol":"BTCUSDC","price":"114497.5","size":"0.0232","side":"Buy","time":"1761520772305","isBlockTrade":false,"isRPITrade":false,"seq":"111156075734"},{"execId":"2240000000970262737","symbol":"BTCUSDC","price":"114492.3","size":"0.03677","side":"Buy","time":"1761520769737","isBlockTrade":false,"isRPITrade":false,"seq":"111156070970"},{"execId":"2240000000970262726","symbol":"BTCUSDC","price":"114471.8","size":"0.004368","side":"Buy","time":"1761520769588","isBlockTrade":false,"isRPITrade":false,"seq":"111156069897"},{"execId":"2240000000970262722","symbol":"BTCUSDC","price":"114471.7","size":"0.000026","side":"Sell","time":"1761520768715","isBlockTrade":false,"isRPITrade":false,"seq":"111156069157"},{"execId":"2240000000970262638","symbol":"BTCUSDC","price":"114472.6","size":"0.03155","side":"Buy","time":"1761520749721","isBlockTrade":false,"isRPITrade":false,"seq":"111156047246"},{"execId":"2240000000970262637","symbol":"BTCUSDC","price":"114472.5","size":"0.01075","side":"Buy","time":"1761520749721","isBlockTrade":false,"isRPITrade":false,"seq":"111156047246"},{"execId":"2240000000970262634","symbol":"BTCUSDC","price":"114483.1","size":"0.010737","side":"Sell","time":"1761520748033","isBlockTrade":false,"isRPITrade":false,"seq":"111156043763"},{"execId":"2240000000970262624","symbol":"BTCUSDC","price":"114494.5","size":"0.00859","side":"Sell","time":"1761520747504","isBlockTrade":false,"isRPITrade":false,"seq":"111156042207"},{"execId":"2240000000970262623","symbol":"BTCUSDC","price":"114500","size":"0.004367","side":"Sell","time":"1761520747084","isBlockTrade":false,"isRPITrade":false,"seq":"111156041389"},{"execId":"2240000000970262622","symbol":"BTCUSDC","price":"114500","size":"0.016936","side":"Sell","time":"1761520747084","isBlockTrade":false,"isRPITrade":false,"seq":"111156041389"},{"execId":"2240000000970262620","symbol":"BTCUSDC","price":"114500","size":"0.000532","side":"Sell","time":"1761520746995","isBlockTrade":false,"isRPITrade":false,"seq":"111156041145"},{"execId":"2240000000970262616","symbol":"BTCUSDC","price":"114505.9","size":"0.004367","side":"Sell","time":"1761520746928","isBlockTrade":false,"isRPITrade":false,"seq":"111156040682"},{"execId":"2240000000970262612","symbol":"BTCUSDC","price":"114517.3","size":"0.003766","side":"Sell","time":"1761520746589","isBlockTrade":false,"isRPITrade":false,"seq":"111156039138"},{"execId":"2240000000970262610","symbol":"BTCUSDC","price":"114517.3","size":"0.0006","side":"Sell","time":"1761520746550","isBlockTrade":false,"isRPITrade":false,"seq":"111156038920"},{"execId":"2240000000970262601","symbol":"BTCUSDC","price":"114532.7","size":"0.024301","side":"Buy","time":"1761520744927","isBlockTrade":false,"isRPITrade":false,"seq":"111156035439"},{"execId":"2240000000970262600","symbol":"BTCUSDC","price":"114532","size":"0.004366","side":"Buy","time":"1761520744927","isBlockTrade":false,"isRPITrade":false,"seq":"111156035439"},{"execId":"2240000000970262599","symbol":"BTCUSDC","price":"114532","size":"0.03695","side":"Buy","time":"1761520744923","isBlockTrade":false,"isRPITrade":false,"seq":"111156035400"},{"execId":"2240000000970262598","symbol":"BTCUSDC","price":"114530.7","size":"0.00001","side":"Buy","time":"1761520744916","isBlockTrade":false,"isRPITrade":false,"seq":"111156035332"},{"execId":"2240000000970262597","symbol":"BTCUSDC","price":"114526.5","size":"0.001472","side":"Buy","time":"1761520744897","isBlockTrade":false,"isRPITrade":false,"seq":"111156035022"},{"execId":"2240000000970262596","symbol":"BTCUSDC","price":"114526.5","size":"0.029748","side":"Buy","time":"1761520744896","isBlockTrade":false,"isRPITrade":false,"seq":"111156035021"},{"execId":"2240000000970262595","symbol":"BTCUSDC","price":"114526.5","size":"0.004197","side":"Buy","time":"1761520744896","isBlockTrade":false,"isRPITrade":false,"seq":"111156035021"},{"execId":"2240000000970262592","symbol":"BTCUSDC","price":"114526.5","size":"0.000032","side":"Buy","time":"1761520743657","isBlockTrade":false,"isRPITrade":false,"seq":"111156033744"},{"execId":"2240000000970262550","symbol":"BTCUSDC","price":"114526.4","size":"0.000038","side":"Sell","time":"1761520738497","isBlockTrade":false,"isRPITrade":false,"seq":"111156029200"},{"execId":"2240000000970262531","symbol":"BTCUSDC","price":"114526.5","size":"0.000137","side":"Buy","time":"1761520733192","isBlockTrade":false,"isRPITrade":false,"seq":"111156025941"},{"execId":"2240000000970262522","symbol":"BTCUSDC","price":"114525.5","size":"0.004628","side":"Buy","time":"1761520729021","isBlockTrade":false,"isRPITrade":false,"seq":"111156022880"},{"execId":"2240000000970262521","symbol":"BTCUSDC","price":"114525.5","size":"0.03122","side":"Buy","time":"1761520729021","isBlockTrade":false,"isRPITrade":false,"seq":"111156022880"},{"execId":"2240000000970262469","symbol":"BTCUSDC","price":"114530.7","size":"0.1012","side":"Sell","time":"1761520714970","isBlockTrade":false,"isRPITrade":false,"seq":"111156011072"},{"execId":"2240000000970262468","symbol":"BTCUSDC","price":"114530.7","size":"0.0006","side":"Sell","time":"1761520714966","isBlockTrade":false,"isRPITrade":false,"seq":"111156011066"},{"execId":"2240000000970262467","symbol":"BTCUSDC","price":"114530.7","size":"0.0125","side":"Sell","time":"1761520714966","isBlockTrade":false,"isRPITrade":false,"seq":"111156011065"},{"execId":"2240000000970262466","symbol":"BTCUSDC","price":"114530.8","size":"0.00003","side":"Buy","time":"1761520713439","isBlockTrade":false,"isRPITrade":false,"seq":"111156009953"},{"execId":"2240000000970262435","symbol":"BTCUSDC","price":"114531.3","size":"0.000037","side":"Sell","time":"1761520708278","isBlockTrade":false,"isRPITrade":false,"seq":"111156004443"},{"execId":"2240000000970262403","symbol":"BTCUSDC","price":"114519.9","size":"0.03122","side":"Buy","time":"1761520698295","isBlockTrade":false,"isRPITrade":false,"seq":"111155994634"},{"execId":"2240000000970262402","symbol":"BTCUSDC","price":"114519.9","size":"0.004366","side":"Buy","time":"1761520698295","isBlockTrade":false,"isRPITrade":false,"seq":"111155994634"},{"execId":"2240000000970262398","symbol":"BTCUSDC","price":"114517.4","size":"0.03122","side":"Buy","time":"1761520698096","isBlockTrade":false,"isRPITrade":false,"seq":"111155994075"},{"execId":"2240000000970262379","symbol":"BTCUSDC","price":"114517.3","size":"0.000099","side":"Sell","time":"1761520691087","isBlockTrade":false,"isRPITrade":false,"seq":"111155989477"},{"execId":"2240000000970262378","symbol":"BTCUSDC","price":"114517.3","size":"0.0006","side":"Sell","time":"1761520691087","isBlockTrade":false,"isRPITrade":false,"seq":"111155989477"},{"execId":"2240000000970262377","symbol":"BTCUSDC","price":"114517.3","size":"0.000066","side":"Sell","time":"1761520691087","isBlockTrade":false,"isRPITrade":false,"seq":"111155989477"},{"execId":"2240000000970262346","symbol":"BTCUSDC","price":"114519.8","size":"0.0003","side":"Sell","time":"1761520690589","isBlockTrade":false,"isRPITrade":false,"seq":"111155988082"},{"execId":"2240000000970262341","symbol":"BTCUSDC","price":"114531.9","size":"0.00383","side":"Sell","time":"1761520690553","isBlockTrade":false,"isRPITrade":false,"seq":"111155987835"},{"execId":"2240000000970262340","symbol":"BTCUSDC","price":"114531.9","size":"0.0037","side":"Sell","time":"1761520690553","isBlockTrade":false,"isRPITrade":false,"seq":"111155987834"},{"execId":"2240000000970262316","symbol":"BTCUSDC","price":"114506","size":"0.0043","side":"Buy","time":"1761520683686","isBlockTrade":false,"isRPITrade":false,"seq":"111155978162"},{"execId":"2240000000970262315","symbol":"BTCUSDC","price":"114506","size":"0.000028","side":"Buy","time":"1761520683215","isBlockTrade":false,"isRPITrade":false,"seq":"111155977352"},{"execId":"2240000000970262303","symbol":"BTCUSDC","price":"114505.9","size":"0.0001","side":"Sell","time":"1761520682399","isBlockTrade":false,"isRPITrade":false,"seq":"111155976449"},{"execId":"2240000000970262270","symbol":"BTCUSDC","price":"114505.9","size":"0.000027","side":"Sell","time":"1761520678059","isBlockTrade":false,"isRPITrade":false,"seq":"111155971855"},{"execId":"2240000000970262215","symbol":"BTCUSDC","price":"114517.3","size":"0.000066","side":"Sell","time":"1761520663682","isBlockTrade":false,"isRPITrade":false,"seq":"111155955576"},{"execId":"2240000000970262213","symbol":"BTCUSDC","price":"114531.9","size":"0.04293","side":"Sell","time":"1761520663576","isBlockTrade":false,"isRPITrade":false,"seq":"111155954902"},{"execId":"2240000000970262212","symbol":"BTCUSDC","price":"114531.9","size":"0.03825","side":"Sell","time":"1761520663573","isBlockTrade":false,"isRPITrade":false,"seq":"111155954870"},{"execId":"2240000000970262211","symbol":"BTCUSDC","price":"114531.9","size":"0.03911","side":"Sell","time":"1761520663568","isBlockTrade":false,"isRPITrade":false,"seq":"111155954854"},{"execId":"2240000000970262210","symbol":"BTCUSDC","price":"114531.9","size":"0.1012","side":"Sell","time":"1761520663563","isBlockTrade":false,"isRPITrade":false,"seq":"111155954821"},{"execId":"2240000000970262209","symbol":"BTCUSDC","price":"114531.9","size":"0.02851","side":"Sell","time":"1761520663562","isBlockTrade":false,"isRPITrade":false,"seq":"111155954817"},{"execId":"2240000000970262208","symbol":"BTCUSDC","price":"114531.9","size":"0.0068","side":"Sell","time":"1761520663561","isBlockTrade":false,"isRPITrade":false,"seq":"111155954813"},{"execId":"2240000000970262198","symbol":"BTCUSDC","price":"114520.6","size":"0.013422","side":"Buy","time":"1761520660830","isBlockTrade":false,"isRPITrade":false,"seq":"111155951410"},{"execId":"2240000000970262197","symbol":"BTCUSDC","price":"114520.6","size":"0.000061","side":"Buy","time":"1761520660830","isBlockTrade":false,"isRPITrade":false,"seq":"111155951410"},{"execId":"2240000000970262192","symbol":"BTCUSDC","price":"114513","size":"0.000072","side":"Buy","time":"1761520660436","isBlockTrade":false,"isRPITrade":false,"seq":"111155949735"},{"execId":"2240000000970262191","symbol":"BTCUSDC","price":"114513","size":"0.0068","side":"Buy","time":"1761520660434","isBlockTrade":false,"isRPITrade":false,"seq":"111155949710"},{"execId":"2240000000970262186","symbol":"BTCUSDC","price":"114499.4","size":"0.000922","side":"Buy","time":"1761520659608","isBlockTrade":false,"isRPITrade":false,"seq":"111155947798"},{"execId":"2240000000970262185","symbol":"BTCUSDC","price":"114499.4","size":"0.004367","side":"Buy","time":"1761520659608","isBlockTrade":false,"isRPITrade":false,"seq":"111155947798"},{"execId":"2240000000970262179","symbol":"BTCUSDC","price":"114490.9","size":"0.004367","side":"Buy","time":"1761520656779","isBlockTrade":false,"isRPITrade":false,"seq":"111155943739"}]},"retExtInfo":{},"time":1761520794180})",
  R"({"retCode":0,"retMsg":"OK","result":{"category":"spot","list":[{"execId":"2240000000970269615","symbol":"BTCUSDC","price":"114676.3","size":"0.01052","side":"Sell","time":"1761521686872","isBlockTrade":false,"isRPITrade":false,"seq":"111157359634"},{"execId":"2240000000970269614","symbol":"BTCUSDC","price":"114676.3","size":"0.01073","side":"Sell","time":"1761521686872","isBlockTrade":false,"isRPITrade":false,"seq":"111157359634"},{"execId":"2240000000970269613","symbol":"BTCUSDC","price":"114661.8","size":"0.0009","side":"Buy","time":"1761521686842","isBlockTrade":false,"isRPITrade":false,"seq":"111157359505"},{"execId":"2240000000970269602","symbol":"BTCUSDC","price":"114665.5","size":"0.000066","side":"Sell","time":"1761521686140","isBlockTrade":false,"isRPITrade":false,"seq":"111157357891"},{"execId":"2240000000970269601","symbol":"BTCUSDC","price":"114667.2","size":"0.00059","side":"Sell","time":"1761521686140","isBlockTrade":false,"isRPITrade":false,"seq":"111157357891"},{"execId":"2240000000970269600","symbol":"BTCUSDC","price":"114667.2","size":"0.00436","side":"Sell","time":"1761521686140","isBlockTrade":false,"isRPITrade":false,"seq":"111157357891"},{"execId":"2240000000970269581","symbol":"BTCUSDC","price":"114667.3","size":"0.000135","side":"Buy","time":"1761521683503","isBlockTrade":false,"isRPITrade":false,"seq":"111157352567"},{"execId":"2240000000970269576","symbol":"BTCUSDC","price":"114676.8","size":"0.00436","side":"Sell","time":"1761521683126","isBlockTrade":false,"isRPITrade":false,"seq":"111157350465"},{"execId":"2240000000970269575","symbol":"BTCUSDC","price":"114676.8","size":"0.025","side":"Sell","time":"1761521683126","isBlockTrade":false,"isRPITrade":false,"seq":"111157350465"},{"execId":"2240000000970269572","symbol":"BTCUSDC","price":"114684.2","size":"0.000616","side":"Buy","time":"1761521683109","isBlockTrade":false,"isRPITrade":false,"seq":"111157350153"},{"execId":"2240000000970269571","symbol":"BTCUSDC","price":"114684.2","size":"0.0009","side":"Buy","time":"1761521683109","isBlockTrade":false,"isRPITrade":false,"seq":"111157350153"},{"execId":"2240000000970269570","symbol":"BTCUSDC","price":"114684.2","size":"0.037584","side":"Buy","time":"1761521683109","isBlockTrade":false,"isRPITrade":false,"seq":"111157350153"},{"execId":"2240000000970269569","symbol":"BTCUSDC","price":"114681.8","size":"0.0009","side":"Buy","time":"1761521683109","isBlockTrade":false,"isRPITrade":false,"seq":"111157350153"},{"execId":"2240000000970269568","symbol":"BTCUSDC","price":"114684.1","size":"0.00436","side":"Sell","time":"1761521683106","isBlockTrade":false,"isRPITrade":false,"seq":"111157350115"},{"execId":"2240000000970269564","symbol":"BTCUSDC","price":"114688.4","size":"0.0009","side":"Buy","time":"1761521682896","isBlockTrade":false,"isRPITrade":false,"seq":"111157348950"},{"execId":"2240000000970269563","symbol":"BTCUSDC","price":"114688.4","size":"0.0013","side":"Buy","time":"1761521682896","isBlockTrade":false,"isRPITrade":false,"seq":"111157348950"},{"execId":"2240000000970269557","symbol":"BTCUSDC","price":"114694.6","size":"0.000031","side":"Buy","time":"1761521680507","isBlockTrade":false,"isRPITrade":false,"seq":"111157345831"},{"execId":"2240000000970269520","symbol":"BTCUSDC","price":"114694.5","size":"0.000037","side":"Sell","time":"1761521675269","isBlockTrade":false,"isRPITrade":false,"seq":"111157337719"},{"execId":"2240000000970269517","symbol":"BTCUSDC","price":"114712.4","size":"0.0013","side":"Buy","time":"1761521673832","isBlockTrade":false,"isRPITrade":false,"seq":"111157334787"},{"execId":"2240000000970269516","symbol":"BTCUSDC","price":"114702","size":"0.0009","side":"Buy","time":"1761521673832","isBlockTrade":false,"isRPITrade":false,"seq":"111157334787"},{"execId":"2240000000970269515","symbol":"BTCUSDC","price":"114702","size":"0.0075","side":"Buy","time":"1761521673832","isBlockTrade":false,"isRPITrade":false,"seq":"111157334787"},{"execId":"2240000000970269510","symbol":"BTCUSDC","price":"114700.5","size":"0.015745","side":"Buy","time":"1761521673602","isBlockTrade":false,"isRPITrade":false,"seq":"111157333709"},{"execId":"2240000000970269509","symbol":"BTCUSDC","price":"114684.2","size":"0.016755","side":"Buy","time":"1761521673602","isBlockTrade":false,"isRPITrade":false,"seq":"111157333709"},{"execId":"2240000000970269508","symbol":"BTCUSDC","price":"114684.2","size":"0.0075","side":"Buy","time":"1761521673602","isBlockTrade":false,"isRPITrade":false,"seq":"111157333709"},{"execId":"2240000000970269505","symbol":"BTCUSDC","price":"114688.3","size":"0.0013","side":"Sell","time":"1761521673440","isBlockTrade":false,"isRPITrade":false,"seq":"111157332691"},{"execId":"2240000000970269501","symbol":"BTCUSDC","price":"114692.9","size":"0.0075","side":"Sell","time":"1761521673356","isBlockTrade":false,"isRPITrade":false,"seq":"111157331762"},{"execId":"2240000000970269500","symbol":"BTCUSDC","price":"114692.9","size":"0.004359","side":"Sell","time":"1761521673356","isBlockTrade":false,"isRPITrade":false,"seq":"111157331762"},{"execId":"2240000000970269488","symbol":"BTCUSDC","price":"114705.2","size":"0.004359","side":"Sell","time":"1761521673149","isBlockTrade":false,"isRPITrade":false,"seq":"111157330066"},{"execId":"2240000000970269487","symbol":"BTCUSDC","price":"114705.2","size":"0.000058","side":"Sell","time":"1761521673149","isBlockTrade":false,"isRPITrade":false,"seq":"111157330066"},{"execId":"2240000000970269486","symbol":"BTCUSDC","price":"114711.1","size":"0.000066","side":"Sell","time":"1761521673134","isBlockTrade":false,"isRPITrade":false,"seq":"111157329840"},{"execId":"2240000000970269485","symbol":"BTCUSDC","price":"114715.3","size":"0.000059","side":"Sell","time":"1761521673125","isBlockTrade":false,"isRPITrade":false,"seq":"111157329732"},{"execId":"2240000000970269484","symbol":"BTCUSDC","price":"114715.3","size":"0.0043","side":"Sell","time":"1761521673122","isBlockTrade":false,"isRPITrade":false,"seq":"111157329694"},{"execId":"2240000000970269483","symbol":"BTCUSDC","price":"114715.3","size":"0.019223","side":"Sell","time":"1761521673115","isBlockTrade":false,"isRPITrade":false,"seq":"111157329563"},{"execId":"2240000000970269482","symbol":"BTCUSDC","price":"114722.5","size":"0.004358","side":"Sell","time":"1761521673112","isBlockTrade":false,"isRPITrade":false,"seq":"111157329535"},{"execId":"2240000000970269481","symbol":"BTCUSDC","price":"114722.5","size":"0.000066","side":"Sell","time":"1761521673112","isBlockTrade":false,"isRPITrade":false,"seq":"111157329535"},{"execId":"2240000000970269463","symbol":"BTCUSDC","price":"114732.1","size":"0.000058","side":"Sell","time":"1761521670672","isBlockTrade":false,"isRPITrade":false,"seq":"111157325646"},{"execId":"2240000000970269462","symbol":"BTCUSDC","price":"114732.1","size":"0.0043","side":"Sell","time":"1761521670581","isBlockTrade":false,"isRPITrade":false,"seq":"111157325186"},{"execId":"2240000000970269439","symbol":"BTCUSDC","price":"114732.2","size":"0.004358","side":"Buy","time":"1761521667679","isBlockTrade":false,"isRPITrade":false,"seq":"111157319464"},{"execId":"2240000000970269343","symbol":"BTCUSDC","price":"114726.2","size":"0.0029","side":"Buy","time":"1761521660643","isBlockTrade":false,"isRPITrade":false,"seq":"111157306264"},{"execId":"2240000000970269342","symbol":"BTCUSDC","price":"114726.2","size":"0.019215","side":"Buy","time":"1761521660643","isBlockTrade":false,"isRPITrade":false,"seq":"111157306264"},{"execId":"2240000000970269341","symbol":"BTCUSDC","price":"114726.2","size":"0.0186","side":"Buy","time":"1761521660643","isBlockTrade":false,"isRPITrade":false,"seq":"111157306260"},{"execId":"2240000000970269340","symbol":"BTCUSDC","price":"114726.2","size":"0.02396","side":"Buy","time":"1761521660640","isBlockTrade":false,"isRPITrade":false,"seq":"111157306248"},{"execId":"2240000000970269339","symbol":"BTCUSDC","price":"114726.2","size":"0.0254","side":"Buy","time":"1761521660640","isBlockTrade":false,"isRPITrade":false,"seq":"111157306247"},{"execId":"2240000000970269338","symbol":"BTCUSDC","price":"114721.3","size":"0.00005","side":"Buy","time":"1761521660528","isBlockTrade":false,"isRPITrade":false,"seq":"111157305942"},{"execId":"2240000000970269337","symbol":"BTCUSDC","price":"114719.9","size":"0.0301","side":"Buy","time":"1761521660527","isBlockTrade":false,"isRPITrade":false,"seq":"111157305941"},{"execId":"2240000000970269336","symbol":"BTCUSDC","price":"114719.9","size":"0.0288","side":"Buy","time":"1761521660527","isBlockTrade":false,"isRPITrade":false,"seq":"111157305940"},{"execId":"2240000000970269322","symbol":"BTCUSDC","price":"114706","size":"0.001519","side":"Buy","time":"1761521657849","isBlockTrade":false,"isRPITrade":false,"seq":"111157300503"},{"execId":"2240000000970269288","symbol":"BTCUSDC","price":"114715.9","size":"0.004359","side":"Sell","time":"1761521655373","isBlockTrade":false,"isRPITrade":false,"seq":"111157295401"},{"execId":"2240000000970269260","symbol":"BTCUSDC","price":"114706.1","size":"0.004359","side":"Buy","time":"1761521652281","isBlockTrade":false,"isRPITrade":false,"seq":"111157287701"},{"execId":"2240000000970269259","symbol":"BTCUSDC","price":"114706.1","size":"0.08","side":"Buy","time":"1761521652281","isBlockTrade":false,"isRPITrade":false,"seq":"111157287701"},{"execId":"2240000000970269258","symbol":"BTCUSDC","price":"114706.1","size":"0.000061","side":"Buy","time":"1761521652281","isBlockTrade":false,"isRPITrade":false,"seq":"111157287701"},{"execId":"2240000000970269231","symbol":"BTCUSDC","price":"114696.8","size":"0.035338","side":"Buy","time":"1761521651925","isBlockTrade":false,"isRPITrade":false,"seq":"111157285231"},{"execId":"2240000000970269230","symbol":"BTCUSDC","price":"114696.8","size":"0.004322","side":"Buy","time":"1761521651925","isBlockTrade":false,"isRPITrade":false,"seq":"111157285231"},{"execId":"2240000000970269218","symbol":"BTCUSDC","price":"114696.8","size":"0.000037","side":"Buy","time":"1761521650284","isBlockTrade":false,"isRPITrade":false,"seq":"111157282466"},{"execId":"2240000000970269196","symbol":"BTCUSDC","price":"114689.8","size":"0.021798","side":"Buy","time":"1761521646172","isBlockTrade":false,"isRPITrade":false,"seq":"111157274332"},{"execId":"2240000000970269195","symbol":"BTCUSDC","price":"114689.8","size":"0.1","side":"Buy","time":"1761521646172","isBlockTrade":false,"isRPITrade":false,"seq":"111157274332"},{"execId":"2240000000970269186","symbol":"BTCUSDC","price":"114689.7","size":"0.000035","side":"Sell","time":"1761521645050","isBlockTrade":false,"isRPITrade":false,"seq":"111157271551"},{"execId":"2240000000970269144","symbol":"BTCUSDC","price":"114676.4","size":"0.08005","side":"Buy","time":"1761521643273","isBlockTrade":false,"isRPITrade":false,"seq":"111157263618"},{"execId":"2240000000970269143","symbol":"BTCUSDC","price":"114676.4","size":"0.01995","side":"Buy","time":"1761521643273","isBlockTrade":false,"isRPITrade":false,"seq":"111157263610"},{"execId":"2240000000970269134","symbol":"BTCUSDC","price":"114680.4","size":"0.000952","side":"Buy","time":"1761521641705","isBlockTrade":false,"isRPITrade":false,"seq":"111157260122"}]},"retExtInfo":{},"time":1761521686903})",
  R"({"retCode":0,"retMsg":"OK","result":{"category":"spot","list":[{"execId":"2240000000970309300","symbol":"BTCUSDC","price":"114687.5","size":"0.000035","side":"Sell","time":"1761526216767","isBlockTrade":false,"isRPITrade":false,"seq":"111164704031"},{"execId":"2240000000970309299","symbol":"BTCUSDC","price":"114688.3","size":"0.000066","side":"Sell","time":"1761526216767","isBlockTrade":false,"isRPITrade":false,"seq":"111164704031"},{"execId":"2240000000970309294","symbol":"BTCUSDC","price":"114690.7","size":"0.0337","side":"Sell","time":"1761526216762","isBlockTrade":false,"isRPITrade":false,"seq":"111164703927"},{"execId":"2240000000970309293","symbol":"BTCUSDC","price":"114691.8","size":"0.00436","side":"Sell","time":"1761526216762","isBlockTrade":false,"isRPITrade":false,"seq":"111164703927"},{"execId":"2240000000970309292","symbol":"BTCUSDC","price":"114691.9","size":"0.0018","side":"Sell","time":"1761526216762","isBlockTrade":false,"isRPITrade":false,"seq":"111164703927"},{"execId":"2240000000970309291","symbol":"BTCUSDC","price":"114692","size":"0.0018","side":"Sell","time":"1761526216762","isBlockTrade":false,"isRPITrade":false,"seq":"111164703927"},{"execId":"2240000000970309290","symbol":"BTCUSDC","price":"114692.1","size":"0.011497","side":"Sell","time":"1761526216762","isBlockTrade":false,"isRPITrade":false,"seq":"111164703927"},{"execId":"2240000000970309289","symbol":"BTCUSDC","price":"114695.6","size":"0.004359","side":"Sell","time":"1761526216762","isBlockTrade":false,"isRPITrade":false,"seq":"111164703927"},{"execId":"2240000000970309288","symbol":"BTCUSDC","price":"114702.2","size":"0.000002","side":"Sell","time":"1761526216759","isBlockTrade":false,"isRPITrade":false,"seq":"111164703907"},{"execId":"2240000000970309287","symbol":"BTCUSDC","price":"114702.2","size":"0.000098","side":"Sell","time":"1761526216759","isBlockTrade":false,"isRPITrade":false,"seq":"111164703905"},{"execId":"2240000000970309286","symbol":"BTCUSDC","price":"114702.2","size":"0.0063","side":"Sell","time":"1761526216759","isBlockTrade":false,"isRPITrade":false,"seq":"111164703905"},{"execId":"2240000000970309285","symbol":"BTCUSDC","price":"114702.2","size":"0.006872","side":"Sell","time":"1761526216759","isBlockTrade":false,"isRPITrade":false,"seq":"111164703905"},{"execId":"2240000000970309284","symbol":"BTCUSDC","price":"114702.2","size":"0.0013","side":"Sell","time":"1761526216759","isBlockTrade":false,"isRPITrade":false,"seq":"111164703905"},{"execId":"2240000000970309283","symbol":"BTCUSDC","price":"114702.2","size":"0.0009","side":"Sell","time":"1761526216759","isBlockTrade":false,"isRPITrade":false,"seq":"111164703905"},{"execId":"2240000000970309261","symbol":"BTCUSDC","price":"114691.9","size":"0.004326","side":"Buy","time":"1761526215338","isBlockTrade":false,"isRPITrade":false,"seq":"111164700516"},{"execId":"2240000000970309258","symbol":"BTCUSDC","price":"114691.9","size":"0.000034","side":"Buy","time":"1761526213661","isBlockTrade":false,"isRPITrade":false,"seq":"111164699235"},{"execId":"2240000000970309225","symbol":"BTCUSDC","price":"114702.5","size":"0.04096","side":"Sell","time":"1761526210196","isBlockTrade":false,"isRPITrade":false,"seq":"111164693245"},{"execId":"2240000000970309211","symbol":"BTCUSDC","price":"114722.5","size":"0.004326","side":"Sell","time":"1761526208979","isBlockTrade":false,"isRPITrade":false,"seq":"111164688946"},{"execId":"2240000000970309198","symbol":"BTCUSDC","price":"114722.5","size":"0.000032","side":"Sell","time":"1761526208022","isBlockTrade":false,"isRPITrade":false,"seq":"111164687321"},{"execId":"2240000000970309189","symbol":"BTCUSDC","price":"114734","size":"0.004358","side":"Sell","time":"1761526206611","isBlockTrade":false,"isRPITrade":false,"seq":"111164683311"},{"execId":"2240000000970309097","symbol":"BTCUSDC","price":"114747","size":"0.000776","side":"Buy","time":"1761526197799","isBlockTrade":false,"isRPITrade":false,"seq":"111164666173"},{"execId":"2240000000970309095","symbol":"BTCUSDC","price":"114739.9","size":"0.000044","side":"Buy","time":"1761526197549","isBlockTrade":false,"isRPITrade":false,"seq":"111164665069"},{"execId":"2240000000970309094","symbol":"BTCUSDC","price":"114739.7","size":"0.087165","side":"Buy","time":"1761526197549","isBlockTrade":false,"isRPITrade":false,"seq":"111164665069"},{"execId":"2240000000970309093","symbol":"BTCUSDC","price":"114737.7","size":"0.0337","side":"Buy","time":"1761526197549","isBlockTrade":false,"isRPITrade":false,"seq":"111164665069"},{"execId":"2240000000970309092","symbol":"BTCUSDC","price":"114737.7","size":"0.007546","side":"Buy","time":"1761526197549","isBlockTrade":false,"isRPITrade":false,"seq":"111164665069"},{"execId":"2240000000970309091","symbol":"BTCUSDC","price":"114737.7","size":"0.004358","side":"Buy","time":"1761526197549","isBlockTrade":false,"isRPITrade":false,"seq":"111164665069"},{"execId":"2240000000970309049","symbol":"BTCUSDC","price":"114737.7","size":"0.020324","side":"Buy","time":"1761526195790","isBlockTrade":false,"isRPITrade":false,"seq":"111164660953"},{"execId":"2240000000970309048","symbol":"BTCUSDC","price":"114735.5","size":"0.000066","side":"Buy","time":"1761526195790","isBlockTrade":false,"isRPITrade":false,"seq":"111164660953"},{"execId":"2240000000970309047","symbol":"BTCUSDC","price":"114734.1","size":"0.004358","side":"Buy","time":"1761526195789","isBlockTrade":false,"isRPITrade":false,"seq":"111164660948"},{"execId":"2240000000970309027","symbol":"BTCUSDC","price":"114722.6","size":"0.031503","side":"Buy","time":"1761526190789","isBlockTrade":false,"isRPITrade":false,"seq":"111164653560"},{"execId":"2240000000970309026","symbol":"BTCUSDC","price":"114722.6","size":"0.004358","side":"Buy","time":"1761526190789","isBlockTrade":false,"isRPITrade":false,"seq":"111164653560"},{"execId":"2240000000970309024","symbol":"BTCUSDC","price":"114734.3","size":"0.007573","side":"Sell","time":"1761526190525","isBlockTrade":false,"isRPITrade":false,"seq":"111164652765"},{"execId":"2240000000970309023","symbol":"BTCUSDC","price":"114734.3","size":"0.004358","side":"Sell","time":"1761526190525","isBlockTrade":false,"isRPITrade":false,"seq":"111164652765"},{"execId":"2240000000970308948","symbol":"BTCUSDC","price":"114734.4","size":"0.000036","side":"Buy","time":"1761526183438","isBlockTrade":false,"isRPITrade":false,"seq":"111164643408"},{"execId":"2240000000970308920","symbol":"BTCUSDC","price":"114734.4","size":"0.000359","side":"Buy","time":"1761526182746","isBlockTrade":false,"isRPITrade":false,"seq":"111164641567"},{"execId":"2240000000970308915","symbol":"BTCUSDC","price":"114734.8","size":"0.000922","side":"Buy","time":"1761526179542","isBlockTrade":false,"isRPITrade":false,"seq":"111164636520"},{"execId":"2240000000970308909","symbol":"BTCUSDC","price":"114728.2","size":"0.01921","side":"Buy","time":"1761526179300","isBlockTrade":false,"isRPITrade":false,"seq":"111164635388"},{"execId":"2240000000970308908","symbol":"BTCUSDC","price":"114722.6","size":"0.000927","side":"Buy","time":"1761526179296","isBlockTrade":false,"isRPITrade":false,"seq":"111164635339"},{"execId":"2240000000970308907","symbol":"BTCUSDC","price":"114722.6","size":"0.004305","side":"Buy","time":"1761526179296","isBlockTrade":false,"isRPITrade":false,"seq":"111164635339"},{"execId":"2240000000970308891","symbol":"BTCUSDC","price":"114722.5","size":"0.000026","side":"Sell","time":"1761526177804","isBlockTrade":false,"isRPITrade":false,"seq":"111164632554"},{"execId":"2240000000970308838","symbol":"BTCUSDC","price":"114722.6","size":"0.000053","side":"Buy","time":"1761526168293","isBlockTrade":false,"isRPITrade":false,"seq":"111164620162"},{"execId":"2240000000970308765","symbol":"BTCUSDC","price":"114735.5","size":"0.000053","side":"Buy","time":"1761526159214","isBlockTrade":false,"isRPITrade":false,"seq":"111164604063"},{"execId":"2240000000970308722","symbol":"BTCUSDC","price":"114722.6","size":"0.000012","side":"Buy","time":"1761526154946","isBlockTrade":false,"isRPITrade":false,"seq":"111164593829"},{"execId":"2240000000970308721","symbol":"BTCUSDC","price":"114722.6","size":"0.004346","side":"Buy","time":"1761526154943","isBlockTrade":false,"isRPITrade":false,"seq":"111164593821"},{"execId":"2240000000970308720","symbol":"BTCUSDC","price":"114722.6","size":"0.031554","side":"Buy","time":"1761526154943","isBlockTrade":false,"isRPITrade":false,"seq":"111164593821"},{"execId":"2240000000970308711","symbol":"BTCUSDC","price":"114722.6","size":"0.000036","side":"Buy","time":"1761526153220","isBlockTrade":false,"isRPITrade":false,"seq":"111164590085"},{"execId":"2240000000970308693","symbol":"BTCUSDC","price":"114722.5","size":"0.000022","side":"Sell","time":"1761526151540","isBlockTrade":false,"isRPITrade":false,"seq":"111164585744"},{"execId":"2240000000970308692","symbol":"BTCUSDC","price":"114722.5","size":"0.000044","side":"Sell","time":"1761526151540","isBlockTrade":false,"isRPITrade":false,"seq":"111164585740"},{"execId":"2240000000970308691","symbol":"BTCUSDC","price":"114723.3","size":"0.000056","side":"Sell","time":"1761526151540","isBlockTrade":false,"isRPITrade":false,"seq":"111164585740"},{"execId":"2240000000970308690","symbol":"BTCUSDC","price":"114723.3","size":"0.004342","side":"Sell","time":"1761526151539","isBlockTrade":false,"isRPITrade":false,"seq":"111164585732"},{"execId":"2240000000970308689","symbol":"BTCUSDC","price":"114723.3","size":"0.004358","side":"Sell","time":"1761526151539","isBlockTrade":false,"isRPITrade":false,"seq":"111164585732"},{"execId":"2240000000970308659","symbol":"BTCUSDC","price":"114738.8","size":"0.000396","side":"Sell","time":"1761526148730","isBlockTrade":false,"isRPITrade":false,"seq":"111164578947"},{"execId":"2240000000970308658","symbol":"BTCUSDC","price":"114738.8","size":"0.003962","side":"Sell","time":"1761526148730","isBlockTrade":false,"isRPITrade":false,"seq":"111164578946"},{"execId":"2240000000970308657","symbol":"BTCUSDC","price":"114738.8","size":"0.000928","side":"Sell","time":"1761526148730","isBlockTrade":false,"isRPITrade":false,"seq":"111164578946"},{"execId":"2240000000970308652","symbol":"BTCUSDC","price":"114745.3","size":"0.004357","side":"Sell","time":"1761526148160","isBlockTrade":false,"isRPITrade":false,"seq":"111164576971"},{"execId":"2240000000970308651","symbol":"BTCUSDC","price":"114745.3","size":"0.000066","side":"Sell","time":"1761526148160","isBlockTrade":false,"isRPITrade":false,"seq":"111164576971"},{"execId":"2240000000970308649","symbol":"BTCUSDC","price":"114759.8","size":"0.000093","side":"Sell","time":"1761526148015","isBlockTrade":false,"isRPITrade":false,"seq":"111164575926"},{"execId":"2240000000970308648","symbol":"BTCUSDC","price":"114759.8","size":"0.003","side":"Sell","time":"1761526148015","isBlockTrade":false,"isRPITrade":false,"seq":"111164575924"},{"execId":"2240000000970308647","symbol":"BTCUSDC","price":"114759.8","size":"0.000346","side":"Sell","time":"1761526148011","isBlockTrade":false,"isRPITrade":false,"seq":"111164575891"},{"execId":"2240000000970308646","symbol":"BTCUSDC","price":"114759.8","size":"0.000918","side":"Sell","time":"1761526148010","isBlockTrade":false,"isRPITrade":false,"seq":"111164575890"}]},"retExtInfo":{},"time":1761526219555})"
};

struct DataSinkTestFixture {
    std::string url = "https://api.bybit.com/v5/market/recent-trade?category=spot&symbol=BTCUSDC";
    std::shared_ptr<SQLite::Database> db;
    std::shared_ptr<AsioScheduler> scheduler;
    std::shared_ptr<connect::context> ctx;

    // Use in-memory database that persists for the lifetime of this object
    DataSinkTestFixture() : db(std::make_shared<SQLite::Database>(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
        , scheduler(AsioScheduler::Create(1))
        , ctx(connect::context::create(scheduler))
    {}

    ~DataSinkTestFixture() = default;
};

// Global fixture instance that persists between tests
static DataSinkTestFixture fixture;

TEST_CASE("Write first", "[datahub][public_trades]")
{
    std::atomic<bool> data_received{false};
    std::atomic<int> callback_count{0};
    std::promise<void> completion_promise;
    auto completion_future = completion_promise.get_future();

    auto btc_usdc_sink = make_data_sink<bybit::PublicTrade, &bybit::PublicTrade::execId, policy::query_on_init<bybit::PublicTrade>>
        (
            fixture.db,
            [&](std::deque<bybit::PublicTrade>&& trades, DataSource source){
                // Handle received trades from server (first run)
                std::cout << "Data sink callback called with " << trades.size() << " trades from "
                          << (source == DataSource::CACHE ? "CACHE" : "SERVER") << std::endl;
                ++callback_count;
                if (!trades.empty()) {
                    data_received = true;
                    completion_promise.set_value();
                } else {
                    std::cout << "Data sink received empty trades list" << std::endl;
                }
            },
            [&](const std::exception_ptr& e) {
                if (e) completion_promise.set_exception(e);
            }
        );

    // Assert basic invariants
    REQUIRE(btc_usdc_sink != nullptr);

    auto entity_acceptor = btc_usdc_sink->data_acceptor<std::deque<bybit::PublicTrade>>();

    auto resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::PublicTrade>>>(
            [=](auto&& resp) {
                entity_acceptor(move(resp.result.list));
            }
        );
    auto dispatcher = make_data_dispatcher(fixture.scheduler, resp_adapter);

    dispatcher(http_samples[0]);

    // Wait for data to be received from server
    auto status = completion_future.wait_for(std::chrono::seconds(1));
    REQUIRE(status == std::future_status::ready);

    // Verify first run behavior: single callback with server data
    REQUIRE(data_received.load() == true);
    REQUIRE(callback_count.load() == 1);
}

TEST_CASE("Write next", "[datahub][public_trades]")
{
    std::atomic<int> callback_count{0};
    std::vector<std::pair<int, size_t>> callback_sequence; // callback_number, trades_count
    std::mutex callback_mutex;
    std::promise<void> completion_promise;
    auto completion_future = completion_promise.get_future();

    auto btc_usdc_sink = make_data_sink<bybit::PublicTrade, &bybit::PublicTrade::execId, policy::query_on_init<bybit::PublicTrade>>
        (
            fixture.db,
            [&](std::deque<bybit::PublicTrade>&& trades, DataSource source){
                std::lock_guard<std::mutex> lock(callback_mutex);
                int current_callback = ++callback_count;
                size_t trades_size = trades.size();
                callback_sequence.emplace_back(current_callback, trades_size);

                std::cout << "Data sink callback #" << current_callback << " called with " << trades_size << " trades from "
                          << (source == DataSource::CACHE ? "CACHE" : "SERVER") << std::endl;

                // Complete after second callback (DB data + network data)
                if (current_callback == 2) {
                    completion_promise.set_value();
                }
            },
            [&](const std::exception_ptr& e) {
                if (e) completion_promise.set_exception(e);
            }
        );

    // Assert basic invariants
    REQUIRE(btc_usdc_sink != nullptr);

    auto entity_acceptor = btc_usdc_sink->data_acceptor<std::deque<bybit::PublicTrade>>();

    auto resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::PublicTrade>>>(
            [=](auto&& resp) {
                entity_acceptor(move(resp.result.list));
            }
        );
    auto dispatcher = make_data_dispatcher(fixture.scheduler, resp_adapter);

    dispatcher(http_samples[1]);

    // Wait for both callbacks (DB data + network data)
    auto status = completion_future.wait_for(std::chrono::seconds(1));
    REQUIRE(status == std::future_status::ready);

    // Verify second run behavior: two callbacks
    REQUIRE(callback_count.load() == 2);
    REQUIRE(callback_sequence.size() == 2);

    // First callback should have DB data (simulated)
    REQUIRE(callback_sequence[0].first == 1);
    REQUIRE(callback_sequence[0].second == 60); // Simulated DB data

    // Second callback should have network data
    REQUIRE(callback_sequence[1].first == 2);
    REQUIRE(callback_sequence[1].second == 60); // Network data

    std::cout << "Callback sequence verified: "
              << "1st callback had " << callback_sequence[0].second << " trades (from DB), "
              << "2nd callback had " << callback_sequence[1].second << " trades (from network)" << std::endl;
}

TEST_CASE("full sync", "[datahub][public_trades][websocket]")
{
    // Track callbacks and data sources
    std::atomic<int> callback_count{0};
    std::mutex callback_mutex;
    std::promise<void> completion_promise;
    auto completion_future = completion_promise.get_future();

    // Create data sink (will load cached data from previous tests)
    auto btc_usdc_sink = make_data_sink<bybit::PublicTrade, &bybit::PublicTrade::execId, policy::query_on_init<bybit::PublicTrade>>(
        fixture.db,
        [&](std::deque<bybit::PublicTrade>&& trades, DataSource source) {
            std::lock_guard<std::mutex> lock(callback_mutex);
            int current_callback = ++callback_count;
            size_t trades_size = trades.size();

            std::cout << "Data sink callback #" << current_callback
                      << " called with " << trades_size << " trades from "
                      << (source == DataSource::CACHE ? "CACHE" : "SERVER") << std::endl;

            // Complete when we have: cache data + websocket data + http historical
            if (callback_count >= 6) {
                completion_promise.set_value();
            }
        },
        [&](const std::exception_ptr& e) {
            if (e) completion_promise.set_exception(e);
        }
    );

    REQUIRE(btc_usdc_sink != nullptr);

    // Create acceptor for the data sink
    auto entity_acceptor = btc_usdc_sink->data_acceptor<std::deque<bybit::PublicTrade>>();

    // Set up websocket adapter and dispatcher (for WebSocket public trade data)
    auto ws_resp_adapter = make_data_adapter<bybit::WsApiPayload<bybit::WsPublicTrade>>(
        [=](auto&& ws_payload) mutable {
            std::cout << "Websocket adapter received " << ws_payload.data.size() << " trades" << std::endl;
            // Convert WsPublicTrade to PublicTrade via deque copy
            std::deque<bybit::PublicTrade> trades;
            for (auto& ws_trade : ws_payload.data) {
                trades.push_back(static_cast<bybit::PublicTrade&>(ws_trade));
            }
            entity_acceptor(std::move(trades));
        }
    );

    auto ws_dispatcher = make_data_dispatcher(fixture.scheduler, ws_resp_adapter);

    // Set up HTTP query adapter and dispatcher (for historical data fill)
    auto http_resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::PublicTrade>>>(
        [=](auto&& resp) mutable {
            std::cout << "HTTP adapter received " << resp.result.list.size() << " historical trades" << std::endl;
            entity_acceptor(std::move(resp.result.list));
        }
    );

    auto http_dispatcher = make_data_dispatcher(fixture.scheduler, http_resp_adapter);

    // Simulate websocket data from samples
    std::cout << "Dispatching websocket sample data..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        ws_dispatcher(websock_samples[i]);
    }

    // Simulate HTTP historical data
    std::cout << "Dispatching HTTP historical data..." << std::endl;
    http_dispatcher(http_samples[2]);

    // Wait for all operations to complete
    auto status = completion_future.wait_for(std::chrono::seconds(1));

    REQUIRE(status == std::future_status::ready);

    // Verify we received data from cache (from previous tests)
    REQUIRE(callback_count.load() == 6);
}

namespace SQLite {
    void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
        std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
        if (apMsg) std::cerr << " (" << apMsg << ")";
        std::cerr << std::endl;
        std::abort(); // Or throw exception if you prefer
    }
}
