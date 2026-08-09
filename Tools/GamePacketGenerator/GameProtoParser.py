import os
import re
import glob


class Packet:
    def __init__(self, name, id, category):
        self.name = name
        self.id = id
        self.category = category   # ex) "Auction", "Battle", "Lobby"


class ProtoParser():
    """
    Game 폴더 안의 *Protocol.proto 파일들을 전부 스캔해서
    GameEnum.proto에 정의된 PacketType 값을 기준으로 패킷 목록을 만든다.

    - recv_pkt : recv_prefix로 시작하는 패킷 (ex: REQ_)
    - send_pkt : send_prefix로 시작하는 패킷 (ex: RES_)
    - total_pkt: 전체 패킷
    - 각 Packet 객체는 category(파일명에서 추출, ex: LobbyProtocol -> Lobby)를 가진다.
    """

    def __init__(self, recv_prefix, send_prefix, prefix='Game', enum_path='GameEnum.proto'):
        self.recv_pkt = []
        self.send_pkt = []
        self.total_pkt = []
        self.recv_prefix = recv_prefix
        self.send_prefix = send_prefix
        self.prefix = prefix          # pb namespace / 클래스 접두 (ex: Game)
        self.includes = []            # 참고용: 스캔된 proto 파일명 (ex: LobbyProtocol)
        self.enum_map = self._parse_enum(enum_path)

    def _parse_enum(self, enum_path):
        """GameEnum.proto를 읽어서 { 'REQ_ENTER_GAME': 1002, ... } 형태로 반환"""
        enum_map = {}
        if not os.path.exists(enum_path):
            raise FileNotFoundError(f'enum 파일을 찾을 수 없습니다: {enum_path}')

        with open(enum_path, 'r', encoding='utf-8') as f:
            for line in f:
                # ex) PKT_REQ_ENTER_GAME = 1002;
                m = re.match(r'\s*PKT_(\w+)\s*=\s*(\d+)\s*;', line)
                if m:
                    enum_map[m.group(1)] = int(m.group(2))
        return enum_map

    def parse_proto(self, path):
        """단일 *Protocol.proto 파일을 파싱해서 recv_pkt/send_pkt/total_pkt에 추가"""
        filename = os.path.basename(path)              # LobbyProtocol.proto
        name = os.path.splitext(filename)[0]            # LobbyProtocol
        category = name.replace("Protocol", "")          # Lobby
        self.includes.append(name)

        with open(path, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        for line in lines:
            stripped = line.strip()
            if not stripped.startswith('message'):
                continue

            pkt_name = stripped.split()[1].strip('{').upper()

            if pkt_name not in self.enum_map:
                raise ValueError(
                    f'[{filename}] message {pkt_name} 이 GameEnum.proto의 PacketType에 등록되어 있지 않습니다. '
                    f'GameEnum.proto에 PKT_{pkt_name} 을 먼저 추가하세요.'
                )

            pkt = Packet(pkt_name, self.enum_map[pkt_name], category)

            if pkt_name.startswith(self.recv_prefix):
                self.recv_pkt.append(pkt)
            elif pkt_name.startswith(self.send_prefix):
                self.send_pkt.append(pkt)
            else:
                continue

            self.total_pkt.append(pkt)

    def parse_all(self, proto_dir='.'):
        """proto_dir 안의 *Protocol.proto 전부 스캔 (GameEnum/GameStruct는 이름 패턴상 자동 제외)"""
        proto_files = sorted(glob.glob(os.path.join(proto_dir, '*Protocol.proto')))
        if not proto_files:
            raise FileNotFoundError(f'{proto_dir} 안에 *Protocol.proto 파일이 없습니다.')

        for path in proto_files:
            self.parse_proto(path)
