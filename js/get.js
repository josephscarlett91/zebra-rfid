async function main() {
  const res = await fetch('https://169.254.231.229/cloud/localRestLogin',{
    method:'GET',
    headers:{
      accept:"application/json",
      Authorization: "Basic YWRtaW46R3RpMjAyNUA="
    }
  });
  const data = await res.json();

  const payload = {
    command: 'get_mode',
    command_id: '16266718797272556',
    payload: {},
  };

  const resp = await fetch('https://169.254.231.229/cloud',{
    method: 'POST',
    headers: {
      accept: 'application/json',
      Authorization: `Bearer ${Object.values(data)[1]}`,
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(payload),
  });
  const postres = await resp.json();
  console.log(postres);
}

main().catch(console.error);
