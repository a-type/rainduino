var Hapi = require("hapi");
var Wreck = require("wreck");

var conditionCodes = {
	chanceflurries : "cfl",
	chancerain     : "cra",
	chancesleet    : "csl",
	chancesnow     : "csn",
	chancetstorms  : "cts",
	clear          : "clr",
	cloudy         : "cld",
	flurries       : "flr",
	fog            : "fog",
	hazy           : "hzy",
	mostlycloudy   : "mcl",
	mostlysunny    : "msn",
	partlycloudy   : "pcl",
	partlysunny    : "psn",
	sleet          : "slt",
	rain           : "ran",
	snow           : "sno",
	sunny          : "sny",
	tstorms        : "tst",
	unknown        : "unk"
};

var server = new Hapi.Server();
server.connection({ port: process.env.PORT });

server.route({
	method : "GET",
	path   : "/forecast",
	handler : function (req, reply) {
		Wreck.get(
			"http://api.wunderground.com/api/" + process.env.API_KEY + "/forecast/q/NC/Raleigh.json",
			{ json : true },
			function (err, response, payload) {
				var hi = payload.forecast.simpleforecast.forecastday[0].high.celsius;
				var low = payload.forecast.simpleforecast.forecastday[0].low.celsius;
				while (hi.length < 3) {
					hi = "0" + hi;
				}
				while (low.length < 3) {
					low = "0" + low;
				}
				var conditions = conditionCodes[payload.forecast.simpleforecast.forecastday[0].icon];
				reply("#" + hi + low + conditions);
			}
		);
	}
})

server.start(function () {
	console.log("Server running at: ", server.info.uri);
});