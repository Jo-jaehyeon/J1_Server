import argparse
import os
from collections import defaultdict

import jinja2

from GameProtoParser import ProtoParser


def write(path, content):
    with open(path, 'w+', encoding='utf-8') as f:
        f.write(content)
    print(f'[WRITE] {path}')


def main():
    arg_parser = argparse.ArgumentParser(description='GamePacketGenerator')
    arg_parser.add_argument('--dir', type=str, default='.', help='proto가 있는 폴더 (기본: 현재 폴더)')
    arg_parser.add_argument('--output', type=str, required=True, help='공통 핸들러 클래스/파일 이름 (ex: GamePacketHandler)')
    arg_parser.add_argument('--recv', type=str, default='REQ_', help='수신 패킷 접두사')
    arg_parser.add_argument('--send', type=str, default='RES_', help='송신 패킷 접두사')
    arg_parser.add_argument('--prefix', type=str, default='Game', help='pb namespace / 접두 (ex: Game)')
    arg_parser.add_argument('--enum', type=str, default='GameEnum.proto', help='PacketType enum이 정의된 proto 파일명')
    arg_parser.add_argument('--templates', type=str, default='../Templates', help='템플릿 폴더 경로')
    args = arg_parser.parse_args()

    parser = ProtoParser(
        recv_prefix=args.recv,
        send_prefix=args.send,
        prefix=args.prefix,
        enum_path=os.path.join(args.dir, args.enum)
    )
    parser.parse_all(args.dir)

    # 카테고리별로 recv_pkt 묶기
    by_category = defaultdict(list)
    for pkt in parser.recv_pkt:
        by_category[pkt.category].append(pkt)
    categories = sorted(by_category.keys())

    # output(예: GamePacketHandler / GamePktHandler)에서 prefix(예: Game)를 뗀 나머지를
    # 카테고리 파일명 접미사로 사용한다.
    #   GamePacketHandler -> suffix "PacketHandler" -> LobbyPacketHandler.h (서버)
    #   GamePktHandler    -> suffix "PktHandler"    -> LobbyPktHandler.h    (클라)
    if args.output.startswith(args.prefix):
        suffix = args.output[len(args.prefix):]
    else:
        suffix = args.output

    file_loader = jinja2.FileSystemLoader(args.templates)
    env = jinja2.Environment(loader=file_loader, trim_blocks=True, lstrip_blocks=True)

    # ------------------------------------------------------------------
    # 1) 공통 GamePacketHandler.h / .cpp
    #    (배열, Init - 카테고리 헤더를 전부 include해서 한 번에 등록, HandlePacket, DispatchPacket)
    #    -> 항상 재생성 (실제 로직이 없는 뼈대 코드이므로 덮어써도 안전)
    # ------------------------------------------------------------------
    h_tmpl = env.get_template('GamePacketHandler_h.jinja')
    write(
        os.path.join(args.dir, f'{args.output}.h'),
        h_tmpl.render(parser=parser, output=args.output, categories=categories, suffix=suffix)
    )

    cpp_tmpl = env.get_template('GamePacketHandler_cpp.jinja')
    write(
        os.path.join(args.dir, f'{args.output}.cpp'),
        cpp_tmpl.render(parser=parser, output=args.output)
    )

    # ------------------------------------------------------------------
    # 2) 카테고리별 파일
    #    - {Category}{suffix}.h   : 함수 선언 (항상 재생성)
    #    - {Category}{suffix}.cpp : 실제 게임 로직 (최초 1회만 생성, 이후 보호)
    #      -> 서버(recv=REQ_)로 실행했을 때만 생성. 등록 코드는 별도 파일 없이
    #         공통 GamePacketHandler.h의 Init()이 직접 처리한다.
    # ------------------------------------------------------------------
    cat_h_tmpl = env.get_template('CategoryPacketHandler_h.jinja')
    cat_impl_tmpl = env.get_template('CategoryPacketHandler_cpp.jinja')

    for category in categories:
        pkts = by_category[category]

        write(
            os.path.join(args.dir, f'{category}{suffix}.h'),
            cat_h_tmpl.render(category=category, pkts=pkts, prefix=args.prefix)
        )

        if args.recv == 'REQ_':
            impl_path = os.path.join(args.dir, f'{category}{suffix}.cpp')
            if os.path.exists(impl_path):
                print(f'[SKIP] {impl_path} 이미 존재함 (기존 로직 보호)')
            else:
                write(
                    impl_path,
                    cat_impl_tmpl.render(category=category, pkts=pkts, prefix=args.prefix, suffix=suffix)
                )

    print('\n완료되었습니다.')


if __name__ == '__main__':
    main()
