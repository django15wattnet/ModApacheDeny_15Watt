import argparse
import urllib.request


def do(url: str, cnt: int):
    idx = 0
    while idx < cnt:
        idx += 1

        # HTTP-Request mit dynamischem User-Agenten senden.
        req = urllib.request.Request(url, headers={'User-Agent': f'UserAgent {idx}'})
        with urllib.request.urlopen(req) as resp:
            print(f'Request {idx}: HTTP {resp.status}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Liest URL und Anzahl aus der Kommandozeile ein.'
    )
    parser.add_argument(
        '--url', required=True, help='Ziel-URL, z. B. https://example.com'
    )
    parser.add_argument('--cnt', required=True, type=int, help='Anzahl der Aufrufe')

    args = parser.parse_args()
    
    do(url=args.url, cnt=args.cnt)
