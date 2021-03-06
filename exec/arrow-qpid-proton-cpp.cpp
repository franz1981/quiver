/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include <proton/connection.hpp>
#include <proton/default_container.hpp>
#include <proton/delivery.hpp>
#include <proton/link.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/tracker.hpp>
#include <proton/value.hpp>
#include <proton/receiver_options.hpp>

#include <assert.h>
#include <chrono>
#include <iostream>
#include <sstream>

long now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
}

void eprint(std::string message) {
    std::cerr << "quiver-arrow: error: " << message << std::endl;
}

struct handler : public proton::messaging_handler {
    std::string connection_mode;
    std::string channel_mode;
    std::string operation;
    std::string id;
    std::string host;
    std::string port;
    std::string path;
    int messages;
    int body_size;
    int credit_window;

    proton::listener listener;
    proton::binary body;

    int sent = 0;
    int received = 0;
    int accepted = 0;

    void on_container_start(proton::container& c) override {
        std::string domain = host + ":" + port;

        proton::connection_options opts;
        opts.sasl_allowed_mechs("ANONYMOUS");

        if (connection_mode == "client") {
            c.connect(domain, opts);
        } else if (connection_mode == "server") {
            listener = c.listen(domain, opts);
        } else {
            throw std::exception();
        }

        body = std::string(body_size, 'x');
    }

    void on_connection_open(proton::connection& c) override {
        if (channel_mode == "active") {
            if (operation == "send") {
                c.open_sender(path);
            } else if (operation == "receive") {
                proton::receiver_options opts;
                opts.credit_window(credit_window);

                c.open_receiver(path, opts);
            } else {
                throw std::exception();
            }
        }
    }

    void on_sendable(proton::sender& s) override {
        assert (operation == "send");

        while (s.credit() && sent < messages) {
            int id = sent + 1;
            long stime = now();

            proton::message m(body);
            m.id(id);
            m.properties().put("SendTime", stime);

            s.send(m);
            sent++;

            std::cout << id << "," << stime << "\n";
        }
    }

    void on_tracker_accept(proton::tracker& t) override {
        accepted++;

        if (accepted == messages) {
            t.connection().close();

            if (connection_mode == "server") {
                listener.stop();
            }
        }
    }

    void on_message(proton::delivery& d, proton::message& m) override {
        assert (operation == "receive");

        if (received == messages) {
            return;
        }

        received++;

        proton::message_id id = m.id();
        proton::scalar stime = m.properties().get("SendTime");
        long rtime = now();

        std::cout << id << "," << stime << "," << rtime << "\n";

        if (received == messages) {
            d.connection().close();

            if (connection_mode == "server") {
                listener.stop();
            }
        }
    }
};

int main(int argc, char** argv) {
    handler h;

    h.connection_mode = argv[1];
    h.channel_mode = argv[2];
    h.operation = argv[3];
    h.id = argv[4];
    h.host = argv[5];
    h.port = argv[6];
    h.path = argv[7];
    h.messages = std::atoi(argv[8]);
    h.body_size = std::atoi(argv[9]);
    h.credit_window = std::atoi(argv[10]);

    if (h.port == "-") {
        h.port = "5672";
    }

    try {
        proton::default_container(h, h.id).run();
    } catch (const std::exception& e) {
        eprint(e.what());
        return 1;
    }

    return 0;
}
