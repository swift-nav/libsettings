use std::boxed::Box;
use std::net::{Shutdown, TcpStream};

use libsettings::client::read_setting;

fn main() {
    let connect_addr = std::env::args().nth(1).expect("no connection string given");
    let group = std::env::args().nth(2).expect("no group given");
    let name = std::env::args().nth(3).expect("no name given");

    eprintln!("Connecting to {}...", connect_addr);
    let the_stream = TcpStream::connect(connect_addr).expect("Unable to connect to remote address");

    let stream_read = the_stream.try_clone().unwrap();
    let stream_write = the_stream.try_clone().unwrap();
    let value = read_setting(
        Box::new(stream_read),
        Box::new(stream_write),
        group.clone(),
        name.clone(),
    );

    the_stream
        .shutdown(Shutdown::Both)
        .expect("Failed to shutdown TCP stream");

    println!("{}.{} = {:?}", group, name, value);
}
