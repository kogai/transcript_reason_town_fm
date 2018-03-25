[%raw "require('isomorphic-fetch')"];

let rss_endpoint = "https://rss.simplecast.com/podcasts/4151/rss";

let job = (x: Feed.item(Feed.enclosure)) => {
  let mp3 = Printf.sprintf("audio/%s", Feed.nameOfUrl(x.Feed.enclosure));
  let flac = Printf.sprintf("%s.flac", Filename.chop_extension(mp3));
  let ffmpeg = Printf.sprintf("ffmpeg -i %s -r 44100 %s", mp3, flac);
  let sox = Printf.sprintf("sox %s --channels-1 --bits=16 %s", flac, flac);
  Async.(
    Feed.download(x.Feed.enclosure)
    |> fmap((_) => Node.Child_process.execSync(ffmpeg))
    |> fmap((_) => Node.Child_process.execSync(sox))
    |> fmap((_) => Storage.default({"keyFilename": "./secret.json"}))
    |> fmap(Storage.bucket(_, "transcript-reason-town-ml"))
    |> fmap(
         Storage.upload(
           _,
           flac,
           (err, file, res) => {
             Js.log(err);
             Js.log(file);
             Js.log(res);
           },
         ),
       )
  );
};

let _x =
  Async.(
    Fetch.fetch(rss_endpoint)
    >>= Fetch.Response.text
    |> fmap(PixlXml.parse(_))
    |> fmap(Feed.tFromJs)
    |> fmap(x => Feed.(x.channel))
    |> fmap(Feed.channelFromJs)
    |> fmap(x => Feed.(x.item))
    |> fmap(Array.to_list)
    |> fmap(List.map(Feed.itemFromJs))
    |> fmap(
         List.map(x => Feed.{...x, enclosure: enclosureFromJs(x.enclosure)}),
       )
    |> fmap(List.hd)
    |> fmap(job)
  );
/* |> fmap(List.map(job)) */
