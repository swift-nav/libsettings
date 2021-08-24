use std::net::TcpStream;

use libsettings::client::Client;

fn main() {
    let connect_addr = std::env::args().nth(1).expect("no connection string given");
    let group = std::env::args().nth(2).expect("no group given");
    let name = std::env::args().nth(3).expect("no name given");

    eprintln!("Connecting to {}...", connect_addr);
    let stream = TcpStream::connect(connect_addr).expect("Unable to connect to remote address");

    let mut client = Client::new(
        stream.try_clone().expect("Unable to clone tcp stream"),
        stream,
    );
    let value = client.read_setting(&group, &name);

    println!("{}.{} = {:?}", group, name, value);
}
