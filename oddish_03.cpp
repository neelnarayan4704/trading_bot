/* HOW TO RUN
   1) Configure things in the Configuration class
   2) Compile: g++ -o bot.exe bot.cpp
   3) Run in loop: while true; do ./bot.exe; sleep 1; done
*/

using namespace std;

/* C includes for networking things */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/* C++ includes */
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>
#include <map>

/* The Configuration class is used to tell the bot how to connect
   to the appropriate exchange. The `test_exchange_index` variable
   only changes the Configuration when `test_mode` is set to `true`.
*/
class Configuration {
private:
  /*
    0 = prod-like
    1 = slower
    2 = empty
  */
  static int const test_exchange_index = 0;
public:
  std::string team_name;
  std::string exchange_hostname;
  int exchange_port;
  /* replace REPLACEME with your team name! */
  Configuration(bool test_mode) : team_name("ODDISH"){
    exchange_port = 20000; /* Default text based port */
    if(true == test_mode) {
      exchange_hostname = "test-exch-" + team_name;
      exchange_port += test_exchange_index;
    } else {
      exchange_hostname = "production";
    }
  }
};

/* Connection establishes a read/write connection to the exchange,
   and facilitates communication with it */
class Connection {
private:
  FILE * in;
  FILE * out;
  int socket_fd;
public:
  Connection(Configuration configuration){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      throw std::runtime_error("Could not create socket");
    }
    std::string hostname = configuration.exchange_hostname;
    hostent *record = gethostbyname(hostname.c_str());
    if(!record) {
      throw std::invalid_argument("Could not resolve host '" + hostname + "'");
    }
    in_addr *address = reinterpret_cast<in_addr *>(record->h_addr);
    std::string ip_address = inet_ntoa(*address);
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip_address.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(configuration.exchange_port);

    int res = connect(sock, ((struct sockaddr *) &server), sizeof(server));
    if (res < 0) {
      throw std::runtime_error("could not connect");
    }
    FILE *exchange_in = fdopen(sock, "r");
    if (exchange_in == NULL){
      throw std::runtime_error("could not open socket for writing");
    }
    FILE *exchange_out = fdopen(sock, "w");
    if (exchange_out == NULL){
      throw std::runtime_error("could not open socket for reading");
    }

    setlinebuf(exchange_in);
    setlinebuf(exchange_out);
    this->in = exchange_in;
    this->out = exchange_out;
    this->socket_fd = res;
  }

  /** Send a string to the server */
  void send_to_exchange(std::string input) {
    std::string line(input);
    /* All messages must always be uppercase */
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
    int res = fprintf(this->out, "%s\n", line.c_str());
    if (res < 0) {
      throw std::runtime_error("error sending to exchange");
    }
  }

  /** Read a line from the server, dropping the newline at the end */
  std::string read_from_exchange()
  {
    /* We assume that no message from the exchange is longer
       than 10,000 chars */
    const size_t len = 10000;
    char buf[len];
    if(!fgets(buf, len, this->in)){
      throw std::runtime_error("reading line from socket");
    }

    int read_length = strlen(buf);
    std::string result(buf);
    /* Chop off the newline */
    result.resize(result.length() - 1);
    return result;
  }
};

/** Join a vector of strings together, with a separator in-between
    each string. This is useful for space-separating things */
std::string join(std::string sep, std::vector<std::string> strs) {
  std::ostringstream stream;
  const int size = strs.size();
  for(int i = 0; i < size; ++i) {
    stream << strs[i];
    if(i != (strs.size() - 1)) {
      stream << sep;
    }
  }
  return stream.str();
}


int main(int argc, char *argv[])
{
    // Be very careful with this boolean! It switches between test and prod
    bool test_mode = false;
    Configuration config(test_mode);
    Connection conn(config);

    std::vector<std::string> data;
    data.push_back(std::string("HELLO"));
    data.push_back(config.team_name);
    /*
      A common mistake people make is to conn.send_to_exchange() > 1
      time for every conn.read_from_exchange() response.
      Since many write messages generate marketdata, this will cause an
      exponential explosion in pending messages. Please, don't do that!
    */
    conn.send_to_exchange(join(" ", data));
    std::string line = conn.read_from_exchange();
    std::cout << "The exchange replied: " << line << std::endl;



    std::vector<int> positions (8,0);
    vector<string> names; 
    names.push_back("BOND");
    names.push_back("GS");
    names.push_back("MS");
    // names.push_back("USD");
    names.push_back("VALBZ");
    names.push_back("VALE");
    names.push_back("WFC");
    names.push_back("XLF");

    map < string, vector < vector < vector < int> > > > book;
    vector<vector<vector<int> > > empty_vec;
    for(int i = 0; i < names.size(); i++) {
      book[names[i]] = empty_vec;
    }
    
    
    // book[symbol][buy=0, sell=1][offer #][price=0,size=1]

    // BOND, GS, MS, USD, VALBZ, VALE, WFC, XLF

    int order = 0;

    while(true) {
      string message = conn.read_from_exchange();

      vector<string> v;
      string word = "";
      
      for(int i = 0; i < message.length(); i++)
      {
        if(message[i] != ' ')
        {
          word += message[i];
        }

        else
        {
          v.push_back(word);
          word = "";
        }
      }

      //// CHECKING IF MARKET HAS CLOSED
      if(v[0]=="CLOSE") {
        std::cout << "The round has ended" << std::endl;
        break;
      }


// vector < vector < vector < vector < int> > > > book (7,vector<vector<vector<int>>>(2));
//     // book[symbol][buy=0, sell=1][offer #][price=0,size=1]

      //// READING THE BOOK
      if(v[0] == "BOOK") 
      {
        //// BOND
        //// EXTREMELY STUPID CODE 
        order++;
        vector<string> bond_order;
        bond_order.clear();
        bond_order.push_back("ADD");
        bond_order.push_back(to_string(order));
        bond_order.push_back("BOND");
        bond_order.push_back("BUY");
        bond_order.push_back(to_string(999));
        bond_order.push_back(to_string(20));
        conn.send_to_exchange(join(" ", bond_order));

        bond_order.clear();
        order++;
        bond_order.push_back("ADD");
        bond_order.push_back(to_string(order));
        bond_order.push_back("BOND");
        bond_order.push_back("SELL");
        bond_order.push_back(to_string(1001));
        bond_order.push_back(to_string(20));
        conn.send_to_exchange(join(" ", bond_order));
      
      // book[symbol][buy=0, sell=1][offer #][price=0,size=1]


        string symbol = v[1];
        
        //only have one sym
      
        // Map<key, value>, <key = symbol, value = price, size>, <key = string, value = <int, int>>, <string, vector<int, int>>
        // key payments = price;
        // value payments = size;
    //     map < string, vector < vector < vector < int> > > > book;
    // // book[symbol][buy=0, sell=1][offer #][price=0,size=1]

        vector<vector<int> > price_size;
        vector<vector<int> > price_size_selling;
        int p = 0;
        vector<vector<vector<int> > > everything;       
        vector<int> t(2,0);
        bool selling = false;

        for(int counter = 3; counter < v.size(); counter++)
        {
          if(v[counter] == "SELL")
          {
            // price_size.clear();
            selling = true;
            // p = 0;
            continue; // OR BREAK?????
          }
          
          int colon = 0;
          string word = v[counter];

          for(int c = 0; c < word.length(); c++)
          {
            if(word[c] == ':')
            {
              colon = c;
              break;
            }
          }

          string first = word.substr(0, colon);
          string second = word.substr(colon+1, word.length() - colon);
          int f = stoi(first);
          int s = stoi(second);
          
          t[0] = f;
          t[1] = s;
          
          if(!selling)
          {
            price_size.push_back(t);
            // p++;
          }
          else
          {
            price_size_selling.push_back(t);
            // p++;
          }

          
          // price_size[p] = f;
          // price_size[p+1] = s;
        }

        // map < string, vector < vector < vector < int> > > > book;

        
        // cout << "EVERYTHING1" << endl;
        everything.push_back(price_size);
        everything.push_back(price_size_selling);
        // cout << "EVERYTHING2" << endl;
        book[symbol] = everything;
        // cout << "EVERYTHING3" << endl;

        //3 bonds, 2 gs, 3 ms, 2 wfc

        if (v[1]=="VALE") {
          if(book["VALBZ"].size() > 0 && book["VALE"].size() > 0) {
          //// Looking for selling VALE, convert buy VALE, buy VALBZ 
            if(book["VALBZ"][1].size() > 0 && book["VALE"][0].size() > 0) {
              vector<int> vale_bid = book["VALE"][0][0];
              vector<int> valbz_ask = book["VALBZ"][1][0];
              int arb_size = min(min(vale_bid[1], valbz_ask[1]), 10);
              int profit = (vale_bid[0] - valbz_ask[0]) * arb_size - 10;
              if(profit>0) {
                cout << "Convert Buy VALE" << endl;
                // SELL VALE
                order++;
                vector<string> vale_order;
                vale_order.clear();
                vale_order.push_back("ADD");
                vale_order.push_back(to_string(order));
                vale_order.push_back("VALE");
                vale_order.push_back("SELL");
                vale_order.push_back(to_string(vale_bid[0]));
                vale_order.push_back(to_string(arb_size));
                conn.send_to_exchange(join(" ", vale_order));
                vale_order.clear();

                vector<string> valbz_order;
                order++;
                valbz_order.push_back("ADD");
                valbz_order.push_back(to_string(order));
                valbz_order.push_back("VALBZ");
                valbz_order.push_back("BUY");
                valbz_order.push_back(to_string(valbz_ask[0]));
                valbz_order.push_back(to_string(arb_size));
                conn.send_to_exchange(join(" ", valbz_order));
                valbz_order.clear();
                
                vector<string> vale_convert;
                order++;
                vale_convert.push_back("CONVERT");
                vale_convert.push_back(to_string(order));
                vale_convert.push_back("VALE");
                vale_convert.push_back("BUY");
                vale_convert.push_back(to_string(arb_size));
                conn.send_to_exchange(join(" ", vale_convert));
                vale_convert.clear();
              }
            }

            //// Looking for buying VALE, convert sell VALE, sell VALBZ
            if(book["VALBZ"][0].size() > 0 && book["VALE"][1].size() > 0) {
              vector<int> vale_ask = book["VALE"][1][0];
              vector<int> valbz_bid = book["VALBZ"][0][0];
              int arb_size = min(min(vale_ask[1], valbz_bid[1]), 10);
              int profit = (valbz_bid[0] - vale_ask[0]) * arb_size - 10;
              if(profit>0) {
                cout << "Convert Sell VALE" << endl;

                // BUY VALE
                order++;
                vector<string> vale_order;
                vale_order.clear();
                vale_order.push_back("ADD");
                vale_order.push_back(to_string(order));
                vale_order.push_back("VALE");
                vale_order.push_back("BUY");
                vale_order.push_back(to_string(vale_ask[0]));
                vale_order.push_back(to_string(arb_size));
                conn.send_to_exchange(join(" ", vale_order));
                vale_order.clear();

                vector<string> valbz_order;
                order++;
                valbz_order.push_back("ADD");
                valbz_order.push_back(to_string(order));
                valbz_order.push_back("VALBZ");
                valbz_order.push_back("SELL");
                valbz_order.push_back(to_string(valbz_bid[0]));
                valbz_order.push_back(to_string(arb_size));
                conn.send_to_exchange(join(" ", valbz_order));
                valbz_order.clear();
                
                vector<string> vale_convert;
                order++;
                vale_convert.push_back("CONVERT");
                vale_convert.push_back(to_string(order));
                vale_convert.push_back("VALE");
                vale_convert.push_back("SELL");
                vale_convert.push_back(to_string(arb_size));
                conn.send_to_exchange(join(" ", vale_convert));
                vale_convert.clear();
              }
            }
          }
        }



        if (v[1]=="XLF") {
          if(book["XLF"].size() > 0 && book["BOND"].size() > 0 && book["GS"].size() > 0 && book["MS"].size() > 0 && book["WFC"].size() > 0) {
          //// Looking for selling XLF, convert XLF, buy underlying 
            if(book["XLF"][0].size() > 0 && book["BOND"][1].size() > 0 && book["GS"][1].size() > 0 && book["MS"][1].size() > 0 && book["WFC"][1].size() > 0) {
              vector<int> xlf = book["XLF"][0][0];
              vector<int> bond = book["BOND"][1][0];
              vector<int> gs = book["GS"][1][0];
              vector<int> ms = book["MS"][1][0];
              vector<int> wfc = book["WFC"][1][0];
              int arb_size = min(min(min(min(xlf[1]/10,bond[1]/3),gs[1]/2),ms[1]/3),wfc[1]/2);
              int profit = 10*xlf[0] - 3*bond[0] - 2*gs[0] - 3*ms[0] - 2*wfc[0];
              if(profit>0) {
                cout << "Sell XLF" << endl;
                // SELL VALE
                order++;
                vector<string> vale_order;
                vale_order.clear();
                vale_order.push_back("ADD");
                vale_order.push_back(to_string(order));
                vale_order.push_back("XLF");
                vale_order.push_back("SELL");
                vale_order.push_back(to_string(xlf[0]));
                vale_order.push_back(to_string(1));
                conn.send_to_exchange(join(" ", vale_order));
                vale_order.clear();
              }
            }

            //// Looking for buying VALE, convert sell VALE, sell VALBZ
            if(book["XLF"][1].size() > 0 && book["BOND"][0].size() > 0 && book["GS"][0].size() > 0 && book["MS"][0].size() > 0 && book["WFC"][0].size() > 0) {
              vector<int> xlf = book["XLF"][1][0];
              vector<int> bond = book["BOND"][0][0];
              vector<int> gs = book["GS"][0][0];
              vector<int> ms = book["MS"][0][0];
              vector<int> wfc = book["WFC"][0][0];
              int arb_size = min(min(min(min(xlf[1]/10,bond[1]/3),gs[1]/2),ms[1]/3),wfc[1]/2);
              int profit = 3*bond[0] + 2*gs[0] + 3*ms[0] + 2*wfc[0] - 10*xlf[0];
              if(profit>0) {
                cout << "Buy XLF" << endl;
                // BUY VALE
                order++;
                vector<string> vale_order;
                vale_order.clear();
                vale_order.push_back("ADD");
                vale_order.push_back(to_string(order));
                vale_order.push_back("XLF");
                vale_order.push_back("BUY");
                vale_order.push_back(to_string(xlf[0]));
                vale_order.push_back(to_string(1));
                conn.send_to_exchange(join(" ", vale_order));
                vale_order.clear();
              }
            }

          }
        }







      }










      if(std::string(message).find("CLOSE") == 0) {
        std::cout << "The round has ended" << std::endl;
        break;
      }
    }
    return 0;
}