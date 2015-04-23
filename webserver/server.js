var Hapi = require("hapi");
var Wreck = require("wreck");

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
				var conditions = payload.forecast.simpleforecast.forecastday[0].icon;
				reply("" + hi + low + conditions);
			}
		);
	}
})

server.start(function () {
	console.log("Server running at: ", server.info.uri);
});