#include "options.hpp"

#include <riemannpp/riemannpp.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace bpo = boost::program_options;
namespace rpp = riemannpp;

using namespace client;
using namespace std;

bpo::options_description 
send_options()
{
	bpo::options_description options("Send Command Options");
	options.add_options()
		( "state,s",         bpo::value<string>(),  "Set the state of the event." )
		( "service,S",       bpo::value<string>(),  "Set the service sending the event." )
		( "host,h",          bpo::value<string>(),  "Set the origin host of the event." )
		( "description,D",   bpo::value<string>(),  "Set the description of the event." )
		( "attribute,a",     bpo::value<string>(),  "Add a new attribute to the event." )
		( "tag,t",           bpo::value<string>(),  "Add a tag to the event." )
		( "metric-sint64,i", bpo::value<int64_t>(), "Set the 64-bit integer metric of the event." )
		( "metric-d,d",      bpo::value<double>(),  "Set the double metric of the event." )
		( "metric-f,f",      bpo::value<float>(),   "Set the float metric of the event." )
		( "ttl,L",           bpo::value<int>(),     "Set the TTL of the event." )
		( "tcp,T",           bpo::value<bool>()->default_value(true), "Send the message over TCP (default)." )
		( "udp,U",           bpo::value<bool>(),    "Send the message over UDP." )
		;
	return options;
}

bpo::options_description
query_options() {
	bpo::options_description options("Query Command Options");
	options.add_options()
		( "query,q", bpo::value<string>(), "Query to send to Riemann." )
		( "json,j",  "Output the results as a JSON array." )
		;
	return options;
}

bpo::options_description
miscellaneous_options() {
	bpo::options_description options("Miscellaneous Options");
	options.add_options()
		( "version,V", "Display version." )
		( "help,?",    "Show this help page." )
		;
	return options;
}

void
process_command_send(const bpo::variables_map& vm) {
	rpp::client_type type = rpp::client_type::tcp;
	if (vm.count("tcp")) {
		type = (vm["tcp"].as<bool>()) ? rpp::client_type::tcp : rpp::client_type::udp;
	} else if (vm.count("udp")) {
		type = (vm["udp"].as<bool>()) ? rpp::client_type::udp : rpp::client_type::tcp;
	}

	rpp::client client;
	client.connect(type, vm["rhost"].as<string>(), vm["rport"].as<int>());

	rpp::event event;
	if (vm.count("state")) {
		rpp::field f(rpp::event_field::state, vm["state"].as<string>());
		event << f;
	}
	if (vm.count("service")) {
		rpp::field f(rpp::event_field::service, vm["service"].as<string>());
		event << f;
	}
	if (vm.count("host")) {
		rpp::field f(rpp::event_field::host, vm["host"].as<string>());
		event << f;
	}
	if (vm.count("description")) {
		rpp::field f(rpp::event_field::description, vm["description"].as<string>());
		event << f;
	}
//	if (vm.count("attribute")) {
//		rpp::attribute attribute;
//		vm["attribute"].as<string>();
//	}
	if (vm.count("tag")) {
		string s(vm["tag"].as<string>());
		event << s;
	}
	if (vm.count("metric-sint64")) {
		rpp::field f(rpp::event_field::metrics64, vm["metric-sint64"].as<string>());
		event << f;
	}
	if (vm.count("metric-dbl")) {
		rpp::field f(rpp::event_field::metricd, vm["metric-dbl"].as<string>());
		event << f;
	}
	if (vm.count("metric-flt")) {
		rpp::field f(rpp::event_field::metricf, vm["metric-flt"].as<string>());
		event << f;
	}
	if (vm.count("ttl")) {
		rpp::field f(rpp::event_field::ttl, vm["ttl"].as<string>());
		event << f;
	}

	rpp::message message;
	message << event;

	client << message;
}

void
process_command_query(const bpo::variables_map& vm) {
	rpp::client client;
	client.connect(rpp::client_type::tcp, vm["rhost"].as<string>(), vm["rport"].as<int>());

	rpp::query query;
	if (vm.count("query")) {
		query.set_string(vm["query"].as<string>());
	}

	rpp::message message;
	message.set_query(query);

	client.send_message_oneshot(message);

	// ops.result_json = (vm.count("json") > 0);
	// client.recv();
}

options 
process_command_line(int argc, char const* argv[]) {
	bpo::positional_options_description arguments;
	arguments.add("command", 1);
	arguments.add("rhost", 1);
	arguments.add("rport", 1);

	bpo::options_description all;
	all.add(miscellaneous_options());
	all.add_options()
		( "command", bpo::value<string>(), "Either query or send" )
		;
	all.add(send_options());
	all.add(query_options());
	all.add_options()
		( "rhost", bpo::value<string>()->default_value("localhost"), "The address of the Riemann server (default: localhost)" )
		( "rport", bpo::value<int>()->default_value(5555),           "The port of the Riemann server (default: 5555)" )
		;

	bpo::variables_map vm;
	options ops;

	try {
		bpo::store(bpo::command_line_parser(argc, argv).options(all).positional(arguments).run(), vm);
	} catch(bpo::too_many_positional_options_error& e) {
		cerr << "Too many arguments provided." << endl;
		ops.show_usage = true;
	} catch(bpo::unknown_option& e) {
		cerr << "Unknown option '" << e.get_option_name() << "'." << endl;
		ops.show_usage = true;
	} catch(bpo::error& e) {
		cerr << "command line parse error: " << e.what() << "'." << endl;
		ops.show_usage = true;
	}

	if ((0 == vm.count("command")) || vm.count("help")) {
		ops.show_usage = true;
	} if (vm.count("command") && (vm["command"].as<string>() == string("query")) && (0 == vm.count("query"))) {
		ops.show_usage = true;
	}

	ops.show_version = (vm.count("version") > 0);
	ops.show_help = (vm.count("help") > 0);

	try {
		if (vm.count("command")) {
			if (vm["command"].as<string>() == "send") {
				process_command_send(vm);
			} else if (vm["command"].as<string>() == "query") {
				process_command_query(vm);
			}
		}
	} catch (rpp::internal_exception &e) {
		cerr << "Error: " << e.error() << " - " << e.reason() << "." << endl;
		ops.show_usage = true;
	}
	return ops;
}

ostream& 
show_usage(ostream& stream) {
	stream << "Usage: riemannpp COMMAND [options...] [HOST] [PORT]" << endl;
	stream << endl;
	stream << "The HOST and PORT arguments are optional, and they default to" << endl;
	stream << "\"localhost\" and 5555, respectively." << endl;
	stream << endl;
	stream << "Command can be either `send` or `query`." << endl;
	return stream;
}

ostream& 
show_help(ostream& stream) {
	stream << "Available commands: send and query." << endl;
	stream << send_options() << endl;
	stream << endl;
	stream << query_options() << endl;
	stream << endl;
	stream << miscellaneous_options() << endl;
	return stream;
}